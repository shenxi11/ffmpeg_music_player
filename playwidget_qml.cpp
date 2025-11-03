#include "playwidget_qml.h"
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QMetaObject>
#include <QPainter>
#include <QPixmap>
#include <QGraphicsBlurEffect>

PlayWidgetQml::PlayWidgetQml(QWidget *parent)
    : QQuickWidget(parent)
    , workerThread(new QThread(this))
    , lrcThread(new QThread(this))
    , pcmThread(new QThread(this))
    , desk(new DeskLrcWidget(nullptr))
{
    // 设置透明背景
    setClearColor(Qt::transparent);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_AlwaysStackOnTop, false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAutoFillBackground(false);

    // 初始化组件
    initializeComponents();

    // 加载 QML 文件
    setSource(QUrl("qrc:/qml/components/PlayWidget.qml"));
    setResizeMode(QQuickWidget::SizeRootObjectToView);

    if (status() == QQuickWidget::Error) {
        qWarning() << "Failed to load PlayWidget.qml:" << errors();
        return;
    } else {
        qDebug() << "PlayWidget.qml loaded successfully, status:" << status();
        qDebug() << "Root object:" << rootObject();
    }

    // 设置 QML 上下文
    setupQmlConnections();
    setupConnections();

    // 启动线程
    workerThread->start();
    lrcThread->start();
    pcmThread->start();
}

PlayWidgetQml::~PlayWidgetQml()
{
    // 停止线程
    if (workerThread && workerThread->isRunning()) {
        workerThread->quit();
        workerThread->wait();
    }
    if (lrcThread && lrcThread->isRunning()) {
        lrcThread->quit();
        lrcThread->wait();
    }
    if (pcmThread && pcmThread->isRunning()) {
        pcmThread->quit();
        pcmThread->wait();
    }

    // 清理桌面歌词
    if (desk) {
        desk->deleteLater();
    }
}

void PlayWidgetQml::initializeComponents()
{
    // 初始化业务逻辑组件
    work = std::make_shared<Worker>();
    lrc = std::make_shared<LrcAnalyze>();
    take_pcm = std::make_shared<TakePcm>();

    // 将组件移动到各自的线程
    work->moveToThread(workerThread);
    lrc->moveToThread(lrcThread);
    take_pcm->moveToThread(pcmThread);

    // 初始化桌面歌词
    desk->show();
    desk->hide();
}

void PlayWidgetQml::setupQmlConnections()
{
    QQuickItem *rootItem = rootObject();
    if (!rootItem) {
        qWarning() << "Failed to get root QML object";
        return;
    }

    // 连接 QML 信号到 C++ 槽
    connect(rootItem, SIGNAL(sliderMoved(int)), this, SLOT(onSliderMoved(int)));
    connect(rootItem, SIGNAL(upClicked()), this, SLOT(onUpClicked()));
}

void PlayWidgetQml::setupConnections()
{
    // Worker 相关连接
    connect(work.get(), &Worker::Stop, this, &PlayWidgetQml::slot_work_stop);
    connect(work.get(), &Worker::Begin, this, &PlayWidgetQml::slot_work_play);
    connect(work.get(), &Worker::durations, [=](qint64 value) {
        this->duration = static_cast<qint64>(value);
        qDebug() << "Worker::durations - Current position:" << value << "microseconds," << (value/1000000) << "seconds";
        setPositionQml(static_cast<double>(value) / 1000000.0);
    });
    // Worker 没有 send_duration 信号，使用 TakePcm::durations 来获取总时长

    // TakePcm 相关连接
    connect(take_pcm.get(), &TakePcm::signal_send_pic_path, this, &PlayWidgetQml::slot_updateBackground);
    connect(take_pcm.get(), &TakePcm::begin_take_lrc, work.get(), &Worker::setPATH);
    connect(take_pcm.get(), &TakePcm::begin_to_play, work.get(), &Worker::play_pcm);
    connect(take_pcm.get(), &TakePcm::data, work.get(), &Worker::receive_data);
    connect(take_pcm.get(), &TakePcm::Position_Change, work.get(), &Worker::reset_play);
    connect(work.get(), &Worker::begin_to_decode, take_pcm.get(), &TakePcm::begin_to_decode);
    connect(take_pcm.get(), &TakePcm::send_totalDuration, work.get(), &Worker::receive_totalDuration);
    connect(take_pcm.get(), &TakePcm::durations, [=](qint64 value) {
        this->duration = static_cast<qint64>(value);
        qDebug() << "TakePcm::durations - Total duration:" << value << "microseconds," << (value/1000000) << "seconds";
        setDurationQml(static_cast<double>(value) / 1000000.0);
    });

    // 歌词相关连接
    connect(lrc.get(), &LrcAnalyze::send_lrc, this, &PlayWidgetQml::slot_Lrc_send_lrc);
    connect(work.get(), &Worker::send_lrc, [=](int lineNumber) {
        highlightLyricLineQml(lineNumber);
        if (lineNumber >= 0 && lineNumber < static_cast<int>(lyrics.size())) {
            auto it = lyrics.find(lineNumber);
            if (it != lyrics.end()) {
                emit signal_desk_lrc(QString::fromStdString(it->second));
            }
        }
    });

    // 桌面歌词连接
    connect(desk, &DeskLrcWidget::signal_play_Clicked, this, &PlayWidgetQml::slot_play_click);
    connect(this, &PlayWidgetQml::signal_playState, [=](ProcessSliderQml::State state) {
        // 转换状态类型给桌面歌词
        if (state == ProcessSliderQml::State::Play) {
            // 桌面歌词显示播放状态
        } else if (state == ProcessSliderQml::State::Pause) {
            // 桌面歌词显示暂停状态
        }
    });
    connect(this, &PlayWidgetQml::signal_desk_lrc, desk, &DeskLrcWidget::slot_receive_lrc);

    // 进度控制连接
    connect(this, &PlayWidgetQml::signal_process_Change, take_pcm.get(), &TakePcm::seekToPosition);
    connect(this, &PlayWidgetQml::signal_filepath, [=](QString path) {
        qDebug() << "PlayWidgetQml::signal_filepath received, emitting to TakePcm:" << path;
        emit take_pcm->signal_begin_make_pcm(path);
    });
    connect(this, &PlayWidgetQml::signal_worker_play, work.get(), &Worker::Pause);
    connect(this, &PlayWidgetQml::signal_remove_click, work.get(), &Worker::reset_status);
}

void PlayWidgetQml::set_isUp(bool flag)
{
    isUp = flag;
    qDebug() << "PlayWidgetQml::set_isUp called with flag:" << flag;
    
    setIsUpQml(flag);
    emit signal_isUpChanged(isUp);
}

void PlayWidgetQml::slot_updateBackground(QString picPath)
{
    qDebug() << "Updating background with image:" << picPath;
    setBackgroundImageQml(picPath);
}

void PlayWidgetQml::_begin_take_lrc(QString str)
{
    // 清空歌词
    lyrics.clear();
    setLyricsQml(QVariantList());
    
    // 开始解析歌词
    QMetaObject::invokeMethod(lrc.get(), [=]() {
        lrc->begin_take_lrc(str);
    }, Qt::QueuedConnection);
}

void PlayWidgetQml::slot_Lrc_send_lrc(const std::map<int, std::string> newLyrics)
{
    lyrics = newLyrics;
    
    // 转换为 QVariantList 发送给 QML
    QVariantList lyricsData;
    for (const auto& pair : lyrics) {
        lyricsData.append(QString::fromStdString(pair.second));
    }
    
    setLyricsQml(lyricsData);
}

void PlayWidgetQml::slot_work_stop()
{
    qDebug() << __FUNCTION__ << "暂停";
    setPlayStateQml(static_cast<int>(ProcessSliderQml::Pause));
    stopRotationQml();
    emit signal_playState(ProcessSliderQml::Pause);
}

void PlayWidgetQml::slot_work_play()
{
    qDebug() << __FUNCTION__ << "播放";
    setPlayStateQml(static_cast<int>(ProcessSliderQml::Play));
    startRotationQml();
    emit signal_playState(ProcessSliderQml::Play);
}

void PlayWidgetQml::slot_play_click()
{
    emit signal_worker_play();
}

void PlayWidgetQml::onSliderMoved(int seconds)
{
    qDebug() << "Slider moved to:" << seconds << "seconds";
    qint64 milliseconds = static_cast<qint64>(seconds) * 1000;
    
    // 重置播放状态
    work->reset_play();
    
    // 发送跳转信号
    emit signal_process_Change(milliseconds, true);
}

void PlayWidgetQml::onUpClicked()
{
    emit signal_big_clicked(true);
}

// QML 接口方法实现
void PlayWidgetQml::setIsUpQml(bool flag)
{
    QMetaObject::invokeMethod(rootObject(), "setIsUp", Q_ARG(QVariant, flag));
}

void PlayWidgetQml::setBackgroundImageQml(const QString &imagePath)
{
    QMetaObject::invokeMethod(rootObject(), "setBackgroundImage", Q_ARG(QVariant, imagePath));
}

void PlayWidgetQml::setSongNameQml(const QString &name)
{
    QMetaObject::invokeMethod(rootObject(), "setSongName", Q_ARG(QVariant, name));
}

void PlayWidgetQml::setLyricsQml(const QVariantList &lyricsData)
{
    QMetaObject::invokeMethod(rootObject(), "setLyrics", Q_ARG(QVariant, QVariant::fromValue(lyricsData)));
}

void PlayWidgetQml::highlightLyricLineQml(int lineNumber)
{
    QMetaObject::invokeMethod(rootObject(), "highlightLyricLine", Q_ARG(QVariant, lineNumber));
}

void PlayWidgetQml::setPlayStateQml(int state)
{
    QMetaObject::invokeMethod(rootObject(), "setPlayState", Q_ARG(QVariant, state));
}

void PlayWidgetQml::setPositionQml(double position)
{
    QMetaObject::invokeMethod(rootObject(), "setPosition", Q_ARG(QVariant, position));
}

void PlayWidgetQml::setDurationQml(double duration)
{
    QMetaObject::invokeMethod(rootObject(), "setDuration", Q_ARG(QVariant, duration));
}

void PlayWidgetQml::startRotationQml()
{
    QMetaObject::invokeMethod(rootObject(), "startRotation");
}

void PlayWidgetQml::stopRotationQml()
{
    QMetaObject::invokeMethod(rootObject(), "stopRotation");
}

// 其他方法的简单实现
void PlayWidgetQml::_play_click(QString songName)
{
    qDebug() << "PlayWidgetQml::_play_click called with songName:" << songName;
    setSongNameQml(songName);
    
    // 开始播放音乐 - 这是关键的缺失部分！
    rePlay(songName);
}

void PlayWidgetQml::_remove_click(QString songName)
{
    emit signal_remove_click();
}

void PlayWidgetQml::openfile()
{
    QWidget dummyWidget;
    QString filePath_ = QFileDialog::getOpenFileName(
        &dummyWidget,
        "Open File",
        QDir::homePath(),
        "Audio Files (*.mp3 *.wav *.flac *.ogg);;All Files (*)"
    );
    
    if (!filePath_.isEmpty()) {
        QFileInfo fileInfo(filePath_);
        emit signal_filepath(filePath_);
    }
}

void PlayWidgetQml::setPianWidgetEnable(bool flag)
{
    // 实现启用/禁用逻辑
}

void PlayWidgetQml::slot_desk_toggled(bool checked)
{
    if (desk) {
        if (checked) {
            desk->show();
        } else {
            desk->hide();
        }
    }
}

bool PlayWidgetQml::checkAndWarnIfPathNotExists(const QString &path)
{
    QFileInfo fileInfo(path);
    return fileInfo.exists() && fileInfo.isFile();
}

void PlayWidgetQml::rePlay(QString path)
{
    if (path.size() == 0)
        return;
    qDebug() << "PlayWidgetQml::rePlay called with path:" << path;
    emit signal_filepath(path);
}