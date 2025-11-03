#include "play_widget.h"
#include <QTime>
#include <QGraphicsBlurEffect>
#include <QPainter>
#include <QUrl>

PlayWidget::PlayWidget(QWidget *parent)
    : QWidget(parent)

{
    //qDebug() << __FUNCTION__ << QThread::currentThread()->currentThreadId;
    
    // 创建背景图片标签（仅用于存储 pixmap，不直接显示）
    backgroundLabel = new QLabel(this);
    backgroundLabel->hide();  // 永远隐藏，只用于存储数据
    
    // 创建组合的进度条和控制栏（所有功能都在一个 QML 中）
    process_slider = new ProcessSliderQml(this);
    process_slider->setFixedSize(1000, 60);
    process_slider->move(0, 500);
    process_slider->setMaxSeconds(0);
    
    // controlBar 现在指向 process_slider（它包含了所有控制功能）
    controlBar = process_slider;
    
    qDebug() << "ProcessSliderQml created at position:" << process_slider->pos() 
             << "size:" << process_slider->size()
             << "visible:" << process_slider->isVisible();

    desk = new DeskLrcQml();
    desk->raise();
    desk->hide();

    music = new QPushButton(this);
    music->setFixedSize(30,30);
    music->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/up.png);"
                "}"
                );
    connect(music,&QPushButton::clicked,this,[=]() {
        emit signal_big_clicked(!isUp);  // 切换状态而不是总是收起
    });
    music->move(10, 10);

    // 创建线程（不要传parent，让线程独立管理生命周期）
    a = new QThread();
    b = new QThread();
    c = new QThread();

    // 创建Worker对象并移动到线程
    work = std::make_shared<Worker>();
    work->moveToThread(c);

    lrc = std::make_shared<LrcAnalyze>();

    // 创建TakePcm对象并移动到线程
    take_pcm = std::make_shared<TakePcm>();
    take_pcm->moveToThread(a);

    // 启动线程
    a->start();
    b->start();
    c->start();
    
    // 验证线程移动是否成功
    qDebug() << "Main thread ID:" << QThread::currentThreadId();
    
    // 通过槽函数验证TakePcm是否在正确线程中运行
    QMetaObject::invokeMethod(take_pcm.get(), [this]() {
        qDebug() << "TakePcm actual thread ID:" << QThread::currentThreadId();
    }, Qt::QueuedConnection);
    init_LyricDisplay();

    QWidget* rotate_widget = new QWidget(this);
    rotatingCircle = new RotatingCircleImage(rotate_widget);
    rotate_widget->move(100, 100);
    rotate_widget->resize(300, 300);
    rotatingCircle->resize(300, 300);

    connect(this, &PlayWidget::signal_stop_rotate, rotatingCircle, &RotatingCircleImage::on_signal_stop_rotate);
    connect(take_pcm.get(), &TakePcm::signal_send_pic_path, process_slider, &ProcessSliderQml::setPicPath);
    connect(take_pcm.get(), &TakePcm::signal_send_pic_path, this, &PlayWidget::slot_updateBackground);  // 同时更新背景
    connect(take_pcm.get(), &TakePcm::signal_send_pic_path, rotatingCircle, &RotatingCircleImage::setImage);  // 同时更新旋转唱片
    connect(take_pcm.get(), &TakePcm::begin_take_lrc, work.get(), &Worker::setPATH);
    connect(take_pcm.get(),&TakePcm::begin_to_play,work.get(),&Worker::play_pcm);
    connect(take_pcm.get(),&TakePcm::data,work.get(),&Worker::receive_data);
    connect(take_pcm.get(),&TakePcm::Position_Change,work.get(),&Worker::reset_play);
    connect(work.get(), &Worker::begin_to_decode, take_pcm.get(), &TakePcm::begin_to_decode);
    connect(take_pcm.get(),&TakePcm::send_totalDuration, work.get(),&Worker::receive_totalDuration);
    connect(take_pcm.get(),&TakePcm::durations,[ = ](qint64 value){
        this->duration = static_cast<qint64>(value);
        qDebug() << "TakePcm::durations - Total duration:" << value << "microseconds," << (value/1000000) << "seconds";
        process_slider->setMaxSeconds(value / 1000000);
    });
    // 连接进度跳转信号
    connect(this, &PlayWidget::signal_process_Change, take_pcm.get(), &TakePcm::seekToPosition);
    connect(process_slider, &ProcessSliderQml::signal_Slider_Move, this, [=](int seconds){
        qDebug() << "Slider moved to:" << seconds << "seconds";
        // signal_process_Change 需要的是毫秒，seekToPosition 会再乘以 1000 转为微秒
        qint64 milliseconds = static_cast<qint64>(seconds) * 1000;
        bool back_flag = false;
        // 重置播放状态
        work->reset_play();
        
        // 发送跳转信号（单位：毫秒）
        emit signal_process_Change(milliseconds, true);
    });
    connect(process_slider, &ProcessSliderQml::signal_up_click, this, [=](){
        emit signal_big_clicked(!isUp);  // 切换状态而不是总是展开
    });

    connect(this, &PlayWidget::signal_filepath, [=](QString path) {
        //emit take_pcm->begin_to_play();
        emit take_pcm->signal_begin_make_pcm(path);
    });
    connect(this, &PlayWidget::signal_worker_play, work.get(), &Worker::Pause);
    connect(this, &PlayWidget::signal_remove_click, work.get(), &Worker::reset_status);

    connect(this, &PlayWidget::signal_filepath, this, &PlayWidget::_begin_take_lrc);
    connect(this,&PlayWidget::signal_begin_take_lrc,lrc.get(),&LrcAnalyze::begin_take_lrc);
    connect(lrc.get(),&LrcAnalyze::send_lrc,work.get(),&Worker::receive_lrc);
    connect(lrc.get(),&LrcAnalyze::send_lrc,this, &PlayWidget::slot_Lrc_send_lrc);
    connect(lyricDisplay, &LyricDisplayQml::signal_current_lrc, this, &PlayWidget::signal_desk_lrc);
    connect(this, &PlayWidget::signal_desk_lrc, desk, &DeskLrcQml::setLyricText);
    
    // 保存歌词更新连接，以便在拖拽时断开
    lyricUpdateConnection = connect(work.get(),&Worker::send_lrc,this,[this](int line){
        if(line != lyricDisplay->currentLine)
        {
            lyricDisplay->highlightLine(line);
            lyricDisplay->scrollToLine(line);  // 直接传递行号，而不是差值
            lyricDisplay->currentLine = line;
            update();
        }
    });
    
    // 连接歌词点击跳转信号
    connect(lyricDisplay, &LyricDisplayQml::signal_lyric_seek, this, &PlayWidget::slot_lyric_seek);


    nameLabel = new QLabel(this);
    nameLabel->move(400, 50);
    nameLabel->setStyleSheet("QLabel { color: white; font-size: 28px; }");
    nameLabel->setWordWrap(true);
    nameLabel->setFixedSize(550, 30);
    nameLabel->setAlignment(Qt::AlignCenter);

    connect(work.get(),&Worker::Stop,this, &PlayWidget::slot_work_stop);
    connect(work.get(),&Worker::Begin,this, &PlayWidget::slot_work_play);

    // 设置桌面歌词的 ProcessSlider 引用，让它直接调用 ControlBar 方法
    desk->setProcessSlider(process_slider);
    connect(desk, &DeskLrcQml::signal_forward_clicked, this, [=](){
        // 快进5秒 - 直接使用 duration 成员变量
        int currentSeconds = process_slider->getState() != ProcessSliderQml::Stop ? 
            static_cast<int>(duration / 1000000) : 0;
        int maxSeconds = static_cast<int>(duration / 1000000);
        int newSeconds = std::min(maxSeconds, currentSeconds + 5);
        emit signal_process_Change(static_cast<qint64>(newSeconds) * 1000000, true);
    });
    connect(desk, &DeskLrcQml::signal_backward_clicked, this, [=](){
        // 快退5秒
        int currentSeconds = process_slider->getState() != ProcessSliderQml::Stop ? 
            static_cast<int>(duration / 1000000) : 0;
        int newSeconds = std::max(0, currentSeconds - 5);
        emit signal_process_Change(static_cast<qint64>(newSeconds) * 1000000, true);
    });
    connect(desk, &DeskLrcQml::signal_close_clicked, this, [=](){
        desk->hide();
        // 更新主界面桌面歌词按钮状态为未选中
        if (process_slider) {
            process_slider->setDeskChecked(false);
            qDebug() << "Desktop lyrics closed via X button - updated main interface button state to unchecked";
        }
    });
    // 更新桌面歌词播放状态
    connect(this, &PlayWidget::signal_playState, this, [=](ProcessSliderQml::State state){
        desk->setPlayingState(state == ProcessSliderQml::Play);
        qDebug() << "Desktop lyric playing state updated to:" << (state == ProcessSliderQml::Play);
    });
    // 连接桌面歌词设置信号 - 打开设置对话框
    connect(desk, &DeskLrcQml::signal_settings_clicked, this, [=](){
        qDebug() << "Desktop lyric settings clicked - opening settings dialog";
        desk->showSettingsDialog();
    });
    
    // ProcessSlider QML 控件连接
    connect(process_slider, &ProcessSliderQml::signal_play_clicked, this, &PlayWidget::slot_play_click);
    connect(this, &PlayWidget::signal_playState, process_slider, &ProcessSliderQml::setState);
    connect(process_slider, &ProcessSliderQml::signal_volumeChanged, work.get(), &Worker::Set_Volume);
    connect(process_slider, &ProcessSliderQml::signal_nextSong, this,[=](){
        emit signal_Next(fileName,get_net_flag());
    });
    connect(process_slider, &ProcessSliderQml::signal_lastSong, this,[=](){
        emit signal_Last(fileName,get_net_flag());
    });
    connect(process_slider, &ProcessSliderQml::signal_stop, this, [=](){
        if(fileName.size())
            emit signal_play_button_click(false, fileName);
    });
    connect(process_slider, &ProcessSliderQml::signal_mlist_toggled, this, &PlayWidget::signal_list_show);
    connect(process_slider, &ProcessSliderQml::signal_rePlay, this, [=](){
        rePlay(this->filePath);
     });
    connect(process_slider, &ProcessSliderQml::signal_desk_toggled, this, &PlayWidget::slot_desk_toggled);
    connect(process_slider, &ProcessSliderQml::signal_loop_change, this, [=](bool isLoop){
        qDebug() << "Loop state changed:" << isLoop;
        // 可以在这里处理循环状态改变的逻辑
    });

    connect(take_pcm.get(), &TakePcm::signal_move, this, [this]() {
        // seek 完成后重新连接 durations（参考原 ProcessSlider 实现）
        qDebug() << "TakePcm::signal_move - 重新连接 durations";
        durationsConnection = connect(work.get(), &Worker::durations, this, [this](qint64 milliseconds){
            int seconds = static_cast<int>(milliseconds / 1000);
            process_slider->setCurrentSeconds(seconds);
        });
        work->slot_setMove();
     });
    connect(take_pcm.get(), &TakePcm::signal_worker_play, work.get(), &Worker::Pause);
    connect(this,&PlayWidget::signal_set_SliderMove,work.get(),&Worker::set_SliderMove);
    
    // 更新进度条当前时间显示
    // Worker::durations 发出的单位是毫秒
    durationsConnection = connect(work.get(), &Worker::durations, this, [this](qint64 milliseconds){
        int seconds = static_cast<int>(milliseconds / 1000);
        process_slider->setCurrentSeconds(seconds);
        work->slot_setMove();
    });

    // 处理滑块按下和松开事件（参考原 ProcessSlider 实现）
    connect(process_slider, &ProcessSliderQml::signal_sliderPressed, [this](){
        qDebug() << "拖动进度条 - 断开 durations 连接";
        // 断开 Worker::durations 连接，避免拖动时进度跳动
        disconnect(durationsConnection);
    });

    // 注意：不在 sliderReleased 时重连，而是在 TakePcm::signal_move 时重连
    // 这样可以避免 seek 期间的时间戳更新干扰

    // 连接播放完成信号
    connect(process_slider, &ProcessSliderQml::signal_playFinished, process_slider, &ProcessSliderQml::slot_playFinished);

    connect(this, &PlayWidget::signal_isUpChanged, process_slider, &ProcessSliderQml::slot_isUpChanged);
}
void PlayWidget::slot_desk_toggled(bool checked){
    if(checked){
        desk->show();
    }else{
        desk->hide();
    }
}
void PlayWidget::set_isUp(bool flag){
    isUp = flag;
    
    qDebug() << "PlayWidget::set_isUp called with flag:" << flag;
    qDebug() << "Call stack trace - isUp:" << flag;
    
    // 控制歌词显示状态
    if (lyricDisplay) {
        if (flag) {
            qDebug() << "Showing lyric display";
            lyricDisplay->show();  // 展开时显示歌词
            lyricDisplay->setIsUp(true);
        } else {
            qDebug() << "Hiding lyric display";
            lyricDisplay->hide();  // 收起时隐藏歌词
            lyricDisplay->setIsUp(false);
        }
    }
    
    // 触发重绘以更新背景
    update();
    
    emit signal_isUpChanged(isUp);
}
void PlayWidget::slot_work_stop(){
    qDebug() << __FUNCTION__ << "暂停";
    emit signal_playState(ProcessSliderQml::Pause);
    if(fileName.size())
    {
        emit signal_stop_rotate(false);
        emit signal_play_button_click(false, fileName);
        process_slider->setSongName(QFileInfo(fileName).baseName());
        nameLabel->setText(QFileInfo(fileName).baseName());
    }
}
void PlayWidget::slot_work_play(){
    emit signal_playState(ProcessSliderQml::Play);
    emit signal_stop_rotate(true);
    emit signal_play_button_click(true, fileName);
    process_slider->setSongName(QFileInfo(fileName).baseName());
    nameLabel->setText(QFileInfo(fileName).baseName());
}
void PlayWidget::slot_Lrc_send_lrc(const std::map<int, std::string> lyrics){
    this->lyricDisplay->currentLine = 5;  // 初始行设为5，对应第一行歌词
    this->lyrics.clear();
    
    {
        std::lock_guard<std::mutex>lock(mtx);
        this->lyrics = lyrics;
    }
    
    // 使用带时间的 std::map 格式设置歌词
    lyricDisplay->setLyrics(lyrics);
    
    // 获取第一句歌词并传给桌面歌词
    if (!lyrics.empty()) {
        auto firstLyric = lyrics.begin();
        QString firstLyricText = QString::fromStdString(firstLyric->second);
        if (!firstLyricText.isEmpty() && firstLyricText.trimmed() != "") {
            qDebug() << "Sending first lyric to desktop:" << firstLyricText;
            desk->setLyricText(firstLyricText);
        } else {
            // 如果第一句是空的，找到第一句非空歌词
            for (const auto& [time, text] : lyrics) {
                QString lyricText = QString::fromStdString(text);
                if (!lyricText.isEmpty() && lyricText.trimmed() != "") {
                    qDebug() << "Sending first non-empty lyric to desktop:" << lyricText;
                    desk->setLyricText(lyricText);
                    break;
                }
            }
        }
    }
    
    // 设置歌曲名字到桌面歌词
    if (!fileName.isEmpty()) {
        QFileInfo fileInfo(fileName);
        QString songName = fileInfo.baseName();
        qDebug() << "Setting desktop song name:" << songName;
        desk->setSongName(songName);
    }
}
void PlayWidget::slot_play_click(){
    qDebug() << "[TIMING] slot_play_click START" << QTime::currentTime().toString("hh:mm:ss.zzz");
    
    // ProcessSliderQml 内部处理进度，这里只需要处理播放状态
    ProcessSliderQml::State currentState = controlBar->getState();
    
    // 检查是否播放完成后首次点击（进度在开头且状态为 Stop）
    // 如果是，则重新播放
    QQuickItem* root = controlBar->rootObject();
    if (root && currentState == ProcessSliderQml::Stop) {
        QVariant currentTime = root->property("currentTime");
        if (currentTime.isValid() && currentTime.toInt() == 0) {
            qDebug() << "播放完成后重新开始，调用 rePlay";
            rePlay(filePath);
            return;
        }
    }
    
    if(currentState == ProcessSliderQml::Pause || currentState == ProcessSliderQml::Stop) {
        emit signal_playState(ProcessSliderQml::Play);
    } else {
        emit signal_playState(ProcessSliderQml::Pause);
    }
    qDebug() << "[TIMING] slot_play_click 发出 signal_worker_play" << QTime::currentTime().toString("hh:mm:ss.zzz");
    emit signal_worker_play();
    qDebug() << "[TIMING] slot_play_click END" << QTime::currentTime().toString("hh:mm:ss.zzz");
    //work->Pause();
}

void PlayWidget::rePlay(QString path)
{
    if(path.size() == 0)
        return;
    emit signal_filepath(path);
}


void PlayWidget::init_LyricDisplay()
{
    qDebug() << "Initializing LyricDisplay...";
    lyricDisplay = new LyricDisplayQml(this);
    lyricDisplay->setClearColor(QColor(Qt::transparent));
    lyricDisplay->setAttribute(Qt::WA_AlwaysStackOnTop,true);
    lyricDisplay->resize(550, 350);
    lyricDisplay->move(400, 100);
    lyricDisplay->hide();  // 初始时隐藏歌词，只有展开时才显示
    qDebug() << "LyricDisplay initialized, size:" << lyricDisplay->size() << "position:" << lyricDisplay->pos();
}


void PlayWidget::_begin_take_lrc(QString str)
{
    this->lyricDisplay->clearLyrics();
    lrc->begin_take_lrc(str);
}
void PlayWidget::openfile()
{

    // 创建一个临时的 QWidget 实例，用于显示文件对话框
    QWidget dummyWidget;

    // 打开文件对话框
    QString filePath_ = QFileDialog::getOpenFileName(
                &dummyWidget,                   // 父窗口（可以是 nullptr）
                "Open File",                    // 对话框标题
                QDir::homePath(),               // 起始目录（可以是任意路径）
                "Audio Files (*.mp3 *.wav *.flac *.ogg);;All Files (*)"  // 文件过滤器
                );

    // 打印选中的文件路径
    if (!filePath_.isEmpty())
    {
        QFileInfo fileInfo(filePath_);
        QString filename = fileInfo.fileName();
        emit signal_add_song(filename,filePath_);
    }
}

void PlayWidget::_play_click(QString songPath)
{

    if(songPath != this->filePath)
    {
        this->filePath = songPath;

        QFileInfo fileInfo(songPath);
        fileName = fileInfo.fileName();
        if(!checkAndWarnIfPathNotExists(songPath))
            return;
        emit signal_filepath(songPath);
    }
    else
    {
        slot_play_click();
    }

}
void PlayWidget::_remove_click(QString songName)
{
    if(songName == this->fileName)
    {
        this->fileName.clear();
        this->filePath.clear();

        emit signal_remove_click();
    }
}
void PlayWidget::setPianWidgetEnable(bool flag)
{
    // PianWidget 现在集成到 ProcessSliderQml 中，不需要单独控制
}
bool PlayWidget::checkAndWarnIfPathNotExists(const QString &path) {

    if (path.startsWith("http", Qt::CaseInsensitive)) {
        qDebug() << "检测到网络路径，跳过存在性检查:" << path;
        return true;
    }

    QFileInfo fileInfo(path);

    if (fileInfo.exists()) {
        return true;
    } else {
        QMessageBox::warning(nullptr,
                             QObject::tr("路径不存在"),
                             QObject::tr("路径不存在或无法访问：\n%1").arg(path));
        return false;
    }
}
PlayWidget::~PlayWidget()
{
    qDebug() << "PlayWidget::~PlayWidget() - Starting cleanup...";
 
    
    // 2. 等待线程完成当前工作
    QThread::msleep(200);
    
    // 3. 退出并等待所有 QThread
    if(a)
    {
        qDebug() << "PlayWidget: Stopping thread a...";
        a->quit();
        if(!a->wait(2000)) {  // 等待最多2秒
            qDebug() << "PlayWidget: Thread a did not finish, terminating...";
            a->terminate();
            a->wait();
        }
    }
    if(b)
    {
        qDebug() << "PlayWidget: Stopping thread b...";
        b->quit();
        if(!b->wait(2000)) {
            qDebug() << "PlayWidget: Thread b did not finish, terminating...";
            b->terminate();
            b->wait();
        }
    }
    if(c)
    {
        qDebug() << "PlayWidget: Stopping thread c...";
        c->quit();
        if(!c->wait(2000)) {
            qDebug() << "PlayWidget: Thread c did not finish, terminating...";
            c->terminate();
            c->wait();
        }
    }
    
    // 4. 清理 shared_ptr（会自动调用析构函数）
    work.reset();
    lrc.reset();
    take_pcm.reset();
    
    qDebug() << "PlayWidget::~PlayWidget() - Cleanup complete";
}

void PlayWidget::slot_updateBackground(QString picPath) {
    qDebug() << "slot_updateBackground called with:" << picPath;
    
    // 如果是 file:/// URL 格式，转换为本地路径
    QString localPath = picPath;
    if (picPath.startsWith("file:///")) {
        localPath = QUrl(picPath).toLocalFile();
        qDebug() << "Converted URL to local path:" << localPath;
    }
    
    // 加载原始图片
    QPixmap originalPixmap(localPath);
    if (originalPixmap.isNull()) {
        qDebug() << "Failed to load album cover for background:" << localPath;
        return;
    }
    
    qDebug() << "Original pixmap size:" << originalPixmap.size();
    
    // 1. 缩放到合适大小（1000x600），保持比例并填充
    QPixmap scaledPixmap = originalPixmap.scaled(1000, 600, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    
    // 2. 裁剪到中心区域
    int x = (scaledPixmap.width() - 1000) / 2;
    int y = (scaledPixmap.height() - 600) / 2;
    QPixmap croppedPixmap = scaledPixmap.copy(x, y, 1000, 600);
    
    qDebug() << "Cropped pixmap size:" << croppedPixmap.size();
    
    // 3. 创建强模糊效果的图片 - 多次缩放模拟高斯模糊
    QImage image = croppedPixmap.toImage();
    
    // 使用更小的尺寸实现更强的模糊效果
    // 第一次模糊：缩小到 100x60
    QImage smallImage1 = image.scaled(100, 60, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    // 第二次模糊：放大到 200x120
    QImage smallImage2 = smallImage1.scaled(200, 120, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    // 第三次模糊：放大到最终尺寸
    QImage blurredImage = smallImage2.scaled(1000, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    
    // 4. 添加更深的半透明深色遮罩，增强对比度和歌词可读性
    QPainter maskPainter(&blurredImage);
    maskPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    maskPainter.fillRect(blurredImage.rect(), QColor(0, 0, 0, 140));  // 增加遮罩透明度从 120 到 140
    maskPainter.end();
    
    // 5. 转换回 QPixmap 并设置
    QPixmap blurredPixmap = QPixmap::fromImage(blurredImage);
    backgroundLabel->setPixmap(blurredPixmap);
    
    // 触发重绘，让 paintEvent 使用新的背景
    update();
    
    qDebug() << "Background updated successfully, pixmap size:" << blurredPixmap.size();
    qDebug() << "backgroundLabel visible:" << backgroundLabel->isVisible() << "geometry:" << backgroundLabel->geometry();
}

void PlayWidget::slot_lyric_seek(int timeMs) {
    qDebug() << "Seeking to time:" << timeMs << "ms";
    
    // 直接使用现有的跳转逻辑，就像进度条拖动一样
    qint64 milliseconds = static_cast<qint64>(timeMs);
    qDebug() << "Converting to milliseconds:" << milliseconds;
    
    // 重置播放状态
    work->reset_play();
    
    // 发送跳转信号（单位：毫秒）
    emit signal_process_Change(milliseconds, true);
    
    // 同步更新进度条显示位置
    if (process_slider) {
        QQuickItem* root = process_slider->rootObject();
        if (root) {
            QVariant totalDuration = root->property("totalDuration");
            if (totalDuration.isValid() && totalDuration.toInt() > 0) {
                double seekRatio = static_cast<double>(timeMs) / (totalDuration.toInt() * 1000);
                root->setProperty("value", seekRatio);
                root->setProperty("currentTime", timeMs / 1000);
            }
        }
    }
}

void PlayWidget::slot_lyric_drag_start() {
    qDebug() << "Lyric drag started - disconnecting lyric update";
    
    // 断开歌词更新信号，避免拖拽时的冲突
    if (lyricUpdateConnection) {
        disconnect(lyricUpdateConnection);
    }
}

void PlayWidget::slot_lyric_drag_end() {
    qDebug() << "Lyric drag ended - reconnecting lyric update";
    
    // 重新连接歌词更新信号
    lyricUpdateConnection = connect(work.get(),&Worker::send_lrc,this,[this](int line){
        if(line != lyricDisplay->currentLine)
        {
            lyricDisplay->highlightLine(line);
            lyricDisplay->scrollToLine(line);
            lyricDisplay->currentLine = line;
            update();
        }
    });
}

void PlayWidget::slot_lyric_preview(int timeMs) {
    // 拖拽预览时显示时间，但不实际跳转
    int seconds = timeMs / 1000;
    int minutes = seconds / 60;
    seconds = seconds % 60;
    
    QString timeStr = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
    qDebug() << "Drag preview time:" << timeStr << "(" << timeMs << "ms)";
    
    // 更新进度条的预览位置，但不实际跳转
    if (process_slider) {
        QQuickItem* root = process_slider->rootObject();
        if (root) {
            QVariant totalDuration = root->property("totalDuration");
            if (totalDuration.isValid() && totalDuration.toInt() > 0) {
                double previewRatio = static_cast<double>(timeMs) / totalDuration.toInt();
                // 只更新视觉位置，不触发跳转
                root->setProperty("previewValue", previewRatio);
            }
        }
    }
}


