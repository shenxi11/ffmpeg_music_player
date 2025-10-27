#include "qml_bridge.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QDebug>

UIBridge::UIBridge(QObject *parent)
    : QObject(parent)
    , m_currentPosition(0)
    , m_totalDuration(0)
    , m_playState(0) // Stop
    , m_isLooping(false)
    , m_volume(50)
    , m_currentLyricLine(-1)
    , m_isMinimized(false)
    , m_showMusicList(false)
    , m_showDeskLyric(false)
    , m_isNetMode(false)
    , m_isLoggedIn(false)
    , m_duration(0)
{
    qDebug() << "UIBridge::UIBridge - Main thread ID:" << QThread::currentThreadId();
    
    initializeComponents();
    setupConnections();
}

UIBridge::~UIBridge()
{
    // 停止线程
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
    if (m_decoderThread) {
        m_decoderThread->quit();
        m_decoderThread->wait();
    }
    if (m_lrcThread) {
        m_lrcThread->quit();
        m_lrcThread->wait();
    }
}

void UIBridge::initializeComponents()
{
    // 创建线程
    m_workerThread = new QThread(this);
    m_decoderThread = new QThread(this);
    m_lrcThread = new QThread(this);
    
    // 创建核心业务组件（复用原有代码）
    m_worker = std::make_shared<Worker>();
    m_takePcm = std::make_shared<TakePcm>();
    m_lrcAnalyze = std::make_shared<LrcAnalyze>();
    m_httpRequest = new HttpRequest(this);
    
    // 移动到线程
    m_worker->moveToThread(m_workerThread);
    m_takePcm->moveToThread(m_decoderThread);
    m_lrcAnalyze->moveToThread(m_lrcThread);
    
    // 启动线程
    m_workerThread->start();
    m_decoderThread->start();
    m_lrcThread->start();
    
    qDebug() << "UIBridge::initializeComponents - Components initialized";
}

void UIBridge::setupConnections()
{
    // ===== 复用原有的信号槽连接 =====
    
    // TakePcm -> Worker 连接
    connect(m_takePcm.get(), &TakePcm::begin_to_play, 
            m_worker.get(), &Worker::play_pcm);
    connect(m_takePcm.get(), &TakePcm::data, 
            m_worker.get(), &Worker::receive_data);
    connect(m_takePcm.get(), &TakePcm::Position_Change, 
            m_worker.get(), &Worker::reset_play);
    connect(m_takePcm.get(), &TakePcm::send_totalDuration, 
            m_worker.get(), &Worker::receive_totalDuration);
    
    // Worker -> TakePcm 连接
    connect(m_worker.get(), &Worker::begin_to_decode, 
            m_takePcm.get(), &TakePcm::begin_to_decode);
    
    // Duration 信号
    connect(m_takePcm.get(), &TakePcm::durations, 
            this, &UIBridge::onDurationChanged);
    
    // Position 信号
    connect(m_worker.get(), &Worker::durations, 
            this, &UIBridge::onPositionChanged);
    
    // 播放状态信号
    connect(m_worker.get(), &Worker::Stop, 
            this, &UIBridge::onWorkStop);
    connect(m_worker.get(), &Worker::Begin, 
            this, &UIBridge::onWorkPlay);
    
    // 歌词相关
    connect(m_takePcm.get(), &TakePcm::begin_take_lrc, 
            m_worker.get(), &Worker::setPATH);
    connect(m_takePcm.get(), &TakePcm::begin_take_lrc, 
            m_lrcAnalyze.get(), &LrcAnalyze::begin_take_lrc);
    connect(m_lrcAnalyze.get(), &LrcAnalyze::send_lrc, 
            m_worker.get(), &Worker::receive_lrc);
    connect(m_lrcAnalyze.get(), &LrcAnalyze::send_lrc, 
            this, &UIBridge::onLrcReceived);
    connect(m_worker.get(), &Worker::send_lrc, 
            this, &UIBridge::onLrcLineChanged);
    
    // 专辑封面
    connect(m_takePcm.get(), &TakePcm::signal_send_pic_path, 
            this, &UIBridge::onAlbumArtChanged);
    
    // Seek 连接
    connect(this, &UIBridge::signal_process_Change, 
            m_takePcm.get(), &TakePcm::seekToPosition);
    connect(this, &UIBridge::signal_set_SliderMove, 
            m_worker.get(), &Worker::set_SliderMove);
    
    // Worker 控制
    connect(this, &UIBridge::signal_worker_play, 
            m_worker.get(), &Worker::Pause);
    connect(this, &UIBridge::signal_remove_click, 
            m_worker.get(), &Worker::reset_status);
    
    // HTTP 相关
    connect(m_httpRequest, &HttpRequest::signal_getusername, 
            this, &UIBridge::onLoginSuccess);
    connect(m_httpRequest, &HttpRequest::signal_addSong_list, 
            this, &UIBridge::onMusicListReceived);
    
    qDebug() << "UIBridge::setupConnections - All connections established";
}

// ===== 槽函数实现 =====

void UIBridge::onDurationChanged(qint64 duration)
{
    m_duration = duration;
    m_totalDuration = static_cast<int>(duration / 1000000); // 转换为秒
    emit totalDurationChanged(m_totalDuration);
}

void UIBridge::onPositionChanged(qint64 position)
{
    m_currentPosition = static_cast<int>(position / 1000); // 转换为秒
    emit currentPositionChanged(m_currentPosition);
}

void UIBridge::onWorkStop()
{
    m_playState = 2; // Pause
    emit playStateChanged(m_playState);
    emit isPlayingChanged(false);
    emit signal_stop_rotate(true);
}

void UIBridge::onWorkPlay()
{
    m_playState = 1; // Play
    emit playStateChanged(m_playState);
    emit isPlayingChanged(true);
    emit signal_stop_rotate(false);
}

void UIBridge::onLrcReceived(const std::map<int, std::string>& lyrics)
{
    m_lyricsMap = lyrics;
    m_allLyrics.clear();
    
    // 转换为QStringList
    for (const auto& pair : lyrics) {
        m_allLyrics.append(QString::fromStdString(pair.second));
    }
    
    emit allLyricsChanged(m_allLyrics);
}

void UIBridge::onLrcLineChanged(int line)
{
    m_currentLyricLine = line;
    emit currentLyricLineChanged(line);
    
    // 更新当前歌词文本
    if (line >= 0 && line < m_allLyrics.size()) {
        m_currentLyric = m_allLyrics[line];
        emit currentLyricChanged(m_currentLyric);
    }
}

void UIBridge::onAlbumArtChanged(QString picPath)
{
    m_currentAlbumArt = picPath;
    emit currentAlbumArtChanged(picPath);
}

void UIBridge::onLoginSuccess(QString username)
{
    m_username = username;
    m_isLoggedIn = true;
    emit usernameChanged(username);
    emit isLoggedInChanged(true);
}

void UIBridge::onMusicListReceived(const QStringList& songList, const QList<double>& durations)
{
    m_netMusicList.clear();
    
    for (int i = 0; i < songList.size(); ++i) {
        QVariantMap item;
        item["name"] = songList[i];
        item["duration"] = (i < durations.size()) ? durations[i] : 0.0;
        item["artist"] = ""; // 可以扩展
        m_netMusicList.append(item);
    }
    
    emit netMusicListChanged(m_netMusicList);
}

// ===== 属性设置器 =====

void UIBridge::setVolume(int vol)
{
    if (m_volume != vol) {
        m_volume = vol;
        emit volumeChanged(vol);
        
        // 调用Worker设置音量
        QMetaObject::invokeMethod(m_worker.get(), "Set_Volume",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, vol));
    }
}

void UIBridge::setIsMinimized(bool mini)
{
    if (m_isMinimized != mini) {
        m_isMinimized = mini;
        emit isMinimizedChanged(mini);
    }
}

void UIBridge::setShowMusicList(bool show)
{
    if (m_showMusicList != show) {
        m_showMusicList = show;
        emit showMusicListChanged(show);
    }
}

void UIBridge::setShowDeskLyric(bool show)
{
    if (m_showDeskLyric != show) {
        m_showDeskLyric = show;
        emit showDeskLyricChanged(show);
    }
}

void UIBridge::setIsNetMode(bool netMode)
{
    if (m_isNetMode != netMode) {
        m_isNetMode = netMode;
        emit isNetModeChanged(netMode);
    }
}

// ===== QML可调用方法实现 =====

void UIBridge::playSong(const QString& path, const QString& name)
{
    qDebug() << "UIBridge::playSong" << path << name;
    
    m_currentSongName = name;
    m_currentSongPath = path;
    
    emit currentSongNameChanged(name);
    emit currentSongPathChanged(path);
    
    // 触发播放
    emit signal_filepath(path);
    emit signal_begin_take_lrc(path);
    
    // 通过信号调用make_pcm
    QMetaObject::invokeMethod(m_takePcm.get(), "make_pcm",
                              Qt::QueuedConnection,
                              Q_ARG(QString, path));
}

void UIBridge::playPause()
{
    qDebug() << "UIBridge::playPause - Current state:" << m_playState;
    
    emit signal_worker_play();
    
    // 状态会通过onWorkStop/onWorkPlay回调更新
}

void UIBridge::stop()
{
    qDebug() << "UIBridge::stop";
    
    QMetaObject::invokeMethod(m_worker.get(), "stopPlayback",
                              Qt::QueuedConnection);
    
    m_playState = 0; // Stop
    emit playStateChanged(m_playState);
    emit isPlayingChanged(false);
}

void UIBridge::nextSong()
{
    qDebug() << "UIBridge::nextSong";
    // 这里需要实现下一首逻辑
    // 可以通过信号通知QML更新列表选中项
}

void UIBridge::previousSong()
{
    qDebug() << "UIBridge::previousSong";
    // 这里需要实现上一首逻辑
}

void UIBridge::seekTo(int position)
{
    qDebug() << "UIBridge::seekTo" << position;
    
    emit signal_set_SliderMove(true);
    
    qint64 newPosition = static_cast<qint64>(position) * m_duration / (1000 * m_totalDuration);
    bool back_flag = (position < m_currentPosition);
    
    emit signal_process_Change(newPosition, back_flag);
    
    emit signal_set_SliderMove(false);
}

void UIBridge::toggleLoop()
{
    m_isLooping = !m_isLooping;
    emit isLoopingChanged(m_isLooping);
}

void UIBridge::forward()
{
    int newPos = qMin(m_totalDuration, m_currentPosition + 5);
    seekTo(newPos);
}

void UIBridge::backward()
{
    int newPos = qMax(0, m_currentPosition - 5);
    seekTo(newPos);
}

void UIBridge::addLocalMusic(const QString& filePath)
{
    qDebug() << "UIBridge::addLocalMusic" << filePath;
    
    QFileInfo info(filePath);
    QString fileName = info.fileName();
    
    // 添加到列表
    QVariantMap item;
    item["name"] = fileName;
    item["path"] = filePath;
    item["duration"] = 0; // 需要解析获取
    
    m_localMusicList.append(item);
    emit localMusicListChanged(m_localMusicList);
    
    emit signal_add_song(fileName, filePath);
}

void UIBridge::removeMusic(const QString& songName)
{
    qDebug() << "UIBridge::removeMusic" << songName;
    
    // 从列表中移除
    for (int i = 0; i < m_localMusicList.size(); ++i) {
        QVariantMap item = m_localMusicList[i].toMap();
        if (item["name"].toString() == songName) {
            m_localMusicList.removeAt(i);
            emit localMusicListChanged(m_localMusicList);
            break;
        }
    }
    
    emit signal_remove_click();
}

QStringList UIBridge::openFileDialog()
{
    QStringList files = QFileDialog::getOpenFileNames(
        nullptr,
        tr("选择音乐文件"),
        QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
        tr("音频文件 (*.mp3 *.wav *.flac *.m4a *.aac *.ogg)")
    );
    
    return files;
}

void UIBridge::refreshLocalMusicList()
{
    qDebug() << "UIBridge::refreshLocalMusicList";
    // 刷新本地列表逻辑
}

void UIBridge::refreshNetMusicList()
{
    qDebug() << "UIBridge::refreshNetMusicList";
    // 触发网络请求
}

void UIBridge::searchMusic(const QString& keyword)
{
    qDebug() << "UIBridge::searchMusic" << keyword;
    
    if (m_httpRequest) {
        m_httpRequest->getMusic(keyword);
    }
}

void UIBridge::downloadMusic(const QString& songName)
{
    qDebug() << "UIBridge::downloadMusic" << songName;
    // 下载逻辑
}

void UIBridge::login(const QString& account, const QString& password)
{
    qDebug() << "UIBridge::login" << account;
    
    if (m_httpRequest) {
        m_httpRequest->Login(account, password);
    }
}

void UIBridge::logout()
{
    m_isLoggedIn = false;
    m_username = "";
    emit isLoggedInChanged(false);
    emit usernameChanged("");
}

void UIBridge::registerAccount(const QString& account, const QString& password, const QString& username)
{
    qDebug() << "UIBridge::registerAccount" << account << username;
    
    if (m_httpRequest) {
        m_httpRequest->Register(account, password, username);
    }
}

QString UIBridge::getLyricAt(int line)
{
    if (line >= 0 && line < m_allLyrics.size()) {
        return m_allLyrics[line];
    }
    return "";
}

void UIBridge::scrollLyricTo(int line)
{
    m_currentLyricLine = line;
    emit currentLyricLineChanged(line);
}

void UIBridge::openTranslatePlugin()
{
    qDebug() << "UIBridge::openTranslatePlugin";
    // 打开翻译插件窗口
}

void UIBridge::openAudioConverter()
{
    qDebug() << "UIBridge::openAudioConverter";
    // 打开音频转换器窗口
}
