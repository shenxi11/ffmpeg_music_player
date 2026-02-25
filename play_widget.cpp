#include "play_widget.h"
#include <QTime>
#include <QGraphicsBlurEffect>
#include <QPainter>
#include <QUrl>

PlayWidget::PlayWidget(QWidget *parent, QWidget *mainWidget)
    : QWidget(parent),
      m_playbackViewModel(new PlaybackViewModel(this)),  // ViewModel是UI层的唆一接口
      currentSession(nullptr),
      lastSeekPosition(0)

{
    //qDebug() << __FUNCTION__ << QThread::currentThread()->currentThreadId;
    
    qDebug() << "[MVVM-UI] PlayWidget: Initializing with ViewModel-only architecture";
    
    // 如果没有传入mainWidget，则使用parent
    QWidget* mainParent = mainWidget ? mainWidget : parent;
    
    // 创建背景图片标签（仅用于存储 pixmap，不直接显示
    backgroundLabel = new QLabel(this);
    backgroundLabel->hide();  // 永远隐藏，只用于存储数据
    
    // 创建组合的进度条和控制栏（所有功能都在一QML 中）
    process_slider = new ProcessSliderQml(this);
    process_slider->setFixedSize(1000, 70);
    process_slider->move(0, 490);
    process_slider->setMaxSeconds(0);
    
    // controlBar 现在指向 process_slider（它包含了所有控制功能）
    controlBar = process_slider;
    
    qDebug() << "ProcessSliderQml created at position:" << process_slider->pos() 
             << "size:" << process_slider->size()
             << "visible:" << process_slider->isVisible();

    desk = new DeskLrcQml(this);
    desk->raise();
    desk->hide();
    
    // 创建播放历史列表，父窗口为mainWidget
    playlistHistory = new PlaylistHistoryQml(mainParent);
    playlistHistory->hide();

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

    // ========== 旧架构初始化（已注释，保留供参考） ==========
    /*
    // 创建线程（不要传parent，让线程独立管理生命周期
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
    */
    
    // ========== 新架构初始化 ==========
    // 创建歌词解析线程（保留）
    b = new QThread();
    lrc = std::make_shared<LrcAnalyze>();
    // lrc 不需要移到线程，或者可以根据需要决
    
    // 初始化音频服务（单例
    qDebug() << "[MVVM-UI] AudioService initialized";
    
    init_LyricDisplay();

    QWidget* rotate_widget = new QWidget(this);
    rotatingCircle = new RotatingCircleImage(rotate_widget);
    rotate_widget->move(100, 100);
    rotate_widget->resize(300, 300);
    rotatingCircle->resize(300, 300);

    connect(this, &PlayWidget::signal_stop_rotate, rotatingCircle, &RotatingCircleImage::on_signal_stop_rotate);
    
    // ========== 旧架构信号连接（已注释，保留供参考） ==========
    /*
    connect(take_pcm.get(), &TakePcm::signal_send_pic_path, process_slider, &ProcessSliderQml::setPicPath);
    connect(take_pcm.get(), &TakePcm::signal_send_pic_path, this, &PlayWidget::slot_updateBackground);
    connect(take_pcm.get(), &TakePcm::signal_send_pic_path, rotatingCircle, &RotatingCircleImage::setImage);
    
    connect(take_pcm.get(), &TakePcm::signal_send_pic_path, this, [this](QString picPath){
        if (!filePath.isEmpty() && !play_net) {
            QString durationStr = QString("%1:%2")
                .arg(duration / 60000000)
                .arg((duration / 1000000) % 60, 2, 10, QChar('0'));
            emit signal_metadata_updated(filePath, picPath, durationStr);
        }
    });
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
    connect(this, &PlayWidget::signal_process_Change, take_pcm.get(), &TakePcm::seekToPosition);
    */
    
    // ========== MVVM架构：连接ViewModel信号到UI ==========
    qDebug() << "[MVVM-UI] Connecting ViewModel signals to UI...";
    
    // 播放状态变
    connect(m_playbackViewModel, &PlaybackViewModel::isPlayingChanged, this, [this]() {
        bool playing = m_playbackViewModel->isPlaying();
        qDebug() << "[MVVM-UI] isPlaying changed:" << playing;
        emit signal_playState(playing ? ProcessSliderQml::Play : ProcessSliderQml::Pause);

        QString currentPath = m_playbackViewModel->currentFilePath();
        if (currentPath.isEmpty()) {
            currentPath = filePath;
        }
        if (playing || !currentPath.isEmpty()) {
            emit signal_play_button_click(playing, currentPath);
        }

        // 同止播放历史列表的播放状
        playlistHistory->updatePlayingState(
            m_playbackViewModel->currentFilePath(),
            playing
        );
    });
    
    // 位置变化
    connect(m_playbackViewModel, &PlaybackViewModel::positionChanged, this, [this]() {
        qint64 positionMs = m_playbackViewModel->position();
        int seconds = static_cast<int>(positionMs / 1000);
        process_slider->setCurrentSeconds(seconds);
    });
    
    // 时长变化
    connect(m_playbackViewModel, &PlaybackViewModel::durationChanged, this, [this]() {
        qint64 durationMs = m_playbackViewModel->duration();
        qDebug() << "[MVVM-UI] Duration changed:" << durationMs << "ms";
        duration = durationMs * 1000;  // 转为微秒保持兼容
        process_slider->setMaxSeconds(durationMs / 1000);
    });
    
    // 当前曲目标题变化
    connect(m_playbackViewModel, &PlaybackViewModel::currentTitleChanged, this, [this]() {
        QString title = m_playbackViewModel->currentTitle();
        if (!title.isEmpty()) {
            qDebug() << "[MVVM-UI] Title changed:" << title;
            process_slider->setSongName(title);
            nameLabel->setText(title);
            currentSongTitle = title;
        }
    });
    
    // 当前艺术家变
    connect(m_playbackViewModel, &PlaybackViewModel::currentArtistChanged, this, [this]() {
        QString artist = m_playbackViewModel->currentArtist();
        if (!artist.isEmpty()) {
            qDebug() << "[MVVM-UI] Artist changed:" << artist;
            currentSongArtist = artist;
        }
    });
    
    // 专辑封面变化
    connect(m_playbackViewModel, &PlaybackViewModel::currentAlbumArtChanged, this, [this]() {
        QString imagePath = m_playbackViewModel->currentAlbumArt();
        qDebug() << "[MVVM-UI] Album art changed:" << imagePath;
        process_slider->setPicPath(imagePath);
        slot_updateBackground(imagePath);
        rotatingCircle->setImage(imagePath);
        QString currentPath = m_playbackViewModel->currentFilePath();
        if (currentPath.isEmpty() && !filePath.isEmpty()) {
            currentPath = filePath;
        }
        
        // 添加到播放历史
        if (!currentPath.isEmpty()) {
            // Prefer fresh ViewModel metadata to avoid reusing previous song values
            QString latestTitle = m_playbackViewModel->currentTitle();
            QString latestArtist = m_playbackViewModel->currentArtist();

            QString title = !latestTitle.isEmpty()
                    ? latestTitle
                    : (currentSongTitle.isEmpty() ? fileName : currentSongTitle);

            QString artist = !latestArtist.isEmpty()
                    ? latestArtist
                    : (!currentSongArtist.isEmpty()
                       ? currentSongArtist
                       : (!networkSongArtist.isEmpty() ? networkSongArtist : QStringLiteral(u"\u672a\u77e5\u827a\u672f\u5bb6")));

            // Never reuse cached previous-song cover here.
            // Decoder's currentAlbumArt belongs to currentPath and arrives asynchronously.
            QString cover = imagePath;
            
            playlistHistory->addSong(currentPath, title, artist, cover);
            qDebug() << "[MVVM-UI] Added to playlist history:" << title << artist;
            
            // 同步当前播放路径到播放历史列
            playlistHistory->setCurrentPlayingPath(currentPath);
        }
        
        // 更新本地音乐列表的元数据
        if (!currentPath.isEmpty() && !play_net) {
            QString durationStr = QString("%1:%2")
                .arg(duration / 60000000)
                .arg((duration / 1000000) % 60, 2, 10, QChar('0'));
            emit signal_metadata_updated(currentPath, imagePath, durationStr);
        }
    });
    
    // 播放开
    connect(m_playbackViewModel, &PlaybackViewModel::playbackStarted, this, [this]() {
        qDebug() << "[MVVM-UI] Playback started";
        emit signal_playState(ProcessSliderQml::Play);

        QString currentPath = m_playbackViewModel->currentFilePath();
        if (currentPath.isEmpty()) {
            currentPath = filePath;
        }
        if (!currentPath.isEmpty()) {
            emit signal_play_button_click(true, currentPath);
        }

        // currentSession通过AudioService获取（内部使用）
        currentSession = AudioService::instance().currentSession();
    });
    
    // 播放停止
    connect(m_playbackViewModel, &PlaybackViewModel::playbackStopped, this, [this]() {
        qDebug() << "[MVVM-UI] Playback stopped";
        emit signal_playState(ProcessSliderQml::Stop);

        QString currentPath = m_playbackViewModel->currentFilePath();
        if (currentPath.isEmpty()) {
            currentPath = filePath;
        }
        emit signal_play_button_click(false, currentPath);

        currentSession = nullptr;
    });
    
    // 旋转动画控制
    connect(m_playbackViewModel, &PlaybackViewModel::shouldStartRotation, this, [this]() {
        emit signal_stop_rotate(true);
    });
    
    connect(m_playbackViewModel, &PlaybackViewModel::shouldStopRotation, this, [this]() {
        emit signal_stop_rotate(false);
    });
    
    // 歌词加载
    connect(m_playbackViewModel, &PlaybackViewModel::shouldLoadLyrics, this, [this](const QString& filePath) {
        qDebug() << "[MVVM-UI] Should load lyrics for:" << filePath;
        this->filePath = filePath;  // 更新当前路径
        _begin_take_lrc(filePath);
    });
    
    qDebug() << "[MVVM-UI] ViewModel signals connected successfully";
    
    // ========== 歌词更新逻辑（使用当前session=========
    // 缓冲状态提示（网络音频卡顿时显示）
    connect(&AudioService::instance(), &AudioService::bufferingStarted,
            this, [this]() {
        qDebug() << "[MVVM-UI] Buffering started";
        if (nameLabel) {
            nameLabel->setText("正在缓冲...");
        }
    });
    
    connect(&AudioService::instance(), &AudioService::bufferingFinished,
            this, [this]() {
        qDebug() << "[MVVM-UI] Buffering finished";
        // 恢复歌曲名显
        if (nameLabel && !fileName.isEmpty()) {
            nameLabel->setText(QFileInfo(fileName).baseName());
        }
    });
    
    // 进度跳转信号连接
    connect(this, &PlayWidget::signal_process_Change, 
            this, [this](qint64 milliseconds, bool back_flag) {
        qDebug() << "Seeking to position:" << milliseconds << "ms";
        // 每次都从AudioService获取当前session，避免使用已销毁的session
        auto session = AudioService::instance().currentSession();
        if (session) {
            session->seekTo(milliseconds);
            lastSeekPosition = milliseconds;
        } else {
            qDebug() << "No active session to seek";
        }
    });
    
    // ========== 进度跳转（通过ViewModel==========
    connect(process_slider, &ProcessSliderQml::signal_Slider_Move, this, [this](int seconds){
        qDebug() << "[MVVM-UI] Slider moved to:" << seconds << "seconds";
        qint64 milliseconds = static_cast<qint64>(seconds) * 1000;

        // 用户拖动音频进度条即表示切回音频焦点并进入播放态。
        emit signal_playState(ProcessSliderQml::Play);

        if (m_playbackViewModel->isPlaying()) {
            m_playbackViewModel->seekTo(milliseconds);
        } else if (m_playbackViewModel->isPaused()) {
            m_playbackViewModel->seekTo(milliseconds);
            m_playbackViewModel->resume();
        } else {
            // Stop态下 seek 不会驱动播放，先恢复当前曲目再定位。
            QUrl resumeUrl = m_playbackViewModel->currentUrl();
            if (resumeUrl.isEmpty()) {
                QString currentPath = m_playbackViewModel->currentFilePath();
                if (currentPath.isEmpty()) {
                    currentPath = filePath;
                }

                if (!currentPath.isEmpty()) {
                    resumeUrl = currentPath.startsWith("http", Qt::CaseInsensitive)
                            ? QUrl(currentPath)
                            : QUrl::fromLocalFile(currentPath);
                }
            }

            if (!resumeUrl.isEmpty()) {
                m_playbackViewModel->play(resumeUrl);
                QTimer::singleShot(0, this, [this, milliseconds]() {
                    m_playbackViewModel->seekTo(milliseconds);
                });
            } else {
                // 回退：没有可恢复曲目时仅执行定位。
                m_playbackViewModel->seekTo(milliseconds);
            }
        }

        // 注意：这里不能再通过 signal_process_Change 触发二次 seek，
        // 否则会导致同一次拖动出现重复 seek，打断 owner handoff。
    });
    
    connect(process_slider, &ProcessSliderQml::signal_up_click, this, [this](){
        emit signal_big_clicked(!isUp);  // 切换状态而不是总是展开
    });
    
    // ========== 歌词分析连接 ==========
    connect(this,&PlayWidget::signal_begin_take_lrc,lrc.get(),&LrcAnalyze::begin_take_lrc);
    connect(lrc.get(),&LrcAnalyze::send_lrc,this, &PlayWidget::slot_Lrc_send_lrc);
    connect(lyricDisplay, &LyricDisplayQml::signal_current_lrc, this, &PlayWidget::signal_desk_lrc);
    connect(this, &PlayWidget::signal_desk_lrc, desk, &DeskLrcQml::setLyricText);
    
    // ========== 歌词更新（基于AudioService的位置变化） ==========
    // 注意：这里仍然直接使用AudioService，因为歌词同步需要精确的时间控制
    lyricUpdateConnection = connect(&AudioService::instance(), &AudioService::positionChanged, 
            this, [this](qint64 positionMs) {
        // 根据时间找到对应的歌词行
        int targetLine = -1;
        // 歌词提前1000ms显示（让歌词切换与播放进度更同步
        int timeInMs = static_cast<int>(positionMs) + 1000;
        
        for (auto it = lyrics.begin(); it != lyrics.end(); ++it) {
            if (it->first <= timeInMs) {
                auto next = std::next(it);
                if (next == lyrics.end() || next->first > timeInMs) {
                    // 找到当前应该高亮的歌
                    targetLine = std::distance(lyrics.begin(), it) + 5;  // +5 对应初始偏移
                    break;
                }
            }
        }
        
        if (targetLine >= 0 && targetLine != lyricDisplay->currentLine) {
            lyricDisplay->highlightLine(targetLine);
            lyricDisplay->scrollToLine(targetLine);
            lyricDisplay->currentLine = targetLine;
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

    // ========== 旧架构状态连接（已注释） ==========
    /*
    connect(work.get(),&Worker::Stop,this, &PlayWidget::slot_work_stop);
    connect(work.get(),&Worker::Begin,this, &PlayWidget::slot_work_play);
    */
    
    // ========== 新架构状态更新已在前面通过 audioService 信号处理 ==========

    // 设置桌面歌词ProcessSlider 引用，让它直接调ControlBar 方法
    desk->setProcessSlider(process_slider);
    connect(desk, &DeskLrcQml::signal_forward_clicked, this, [=](){
        // 快进5- 直接使用 duration 成员变量
        int currentSeconds = process_slider->getState() != ProcessSliderQml::Stop ? 
            static_cast<int>(duration / 1000000) : 0;
        int maxSeconds = static_cast<int>(duration / 1000000);
        int newSeconds = std::min(maxSeconds, currentSeconds + 5);
        emit signal_process_Change(static_cast<qint64>(newSeconds) * 1000000, true);
    });
    connect(desk, &DeskLrcQml::signal_backward_clicked, this, [=](){
        // 快退5
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
    // 更新桌面歌词播放状
    connect(this, &PlayWidget::signal_playState, this, [=](ProcessSliderQml::State state){
        desk->setPlayingState(state == ProcessSliderQml::Play);
        qDebug() << "Desktop lyric playing state updated to:" << (state == ProcessSliderQml::Play);
    });
    // 连接桌面歌词设置信号 - 打开设置对话
    connect(desk, &DeskLrcQml::signal_settings_clicked, this, [=](){
        qDebug() << "Desktop lyric settings clicked - opening settings dialog";
        desk->showSettingsDialog();
    });
    
    // ProcessSlider QML 控件连接
    connect(process_slider, &ProcessSliderQml::signal_play_clicked, this, &PlayWidget::slot_play_click);
    connect(this, &PlayWidget::signal_playState, process_slider, &ProcessSliderQml::setState);
    
    // ========== 旧架构音量控制（已注释） ==========
    /*
    connect(process_slider, &ProcessSliderQml::signal_volumeChanged, work.get(), &Worker::Set_Volume);
    */
    
    // ========== 新架构音量控==========
    connect(process_slider, &ProcessSliderQml::signal_volumeChanged, this, [this](int volume) {
        qDebug() << "[MVVM-UI] Volume changed to:" << volume;
        m_playbackViewModel->setVolume(volume);
    });
    
    // ========== 上一下一==========
    connect(process_slider, &ProcessSliderQml::signal_nextSong, this, [this](){
        qDebug() << "[MVVM-UI] Next song clicked";
        m_playbackViewModel->playNext();
    });
    connect(process_slider, &ProcessSliderQml::signal_lastSong, this, [this](){
        qDebug() << "[MVVM-UI] Previous song clicked";
        m_playbackViewModel->playPrevious();
    });
    connect(process_slider, &ProcessSliderQml::signal_stop, this, [=](){
        if(filePath.size())
            emit signal_play_button_click(false, filePath);
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
    
    // ========== 播放历史列表 ==========
    connect(process_slider, &ProcessSliderQml::signal_mlist_toggled, this, [this](bool show){
        qDebug() << "Playlist toggle:" << show;
        if (show) {
            // 计算播放列表位置（相对于 mainWidget
            QWidget* mainWidget = playlistHistory->parentWidget();
            if (mainWidget) {
                int windowX = mainWidget->width() - 400;  // 贴在主窗口右边缘
                int windowY = 60;  // 距离顶部60px（topWidget高度
                int windowHeight = mainWidget->height() - 70 - 65;  // 高度：主窗口高度 - slider高度 - topWidget高度
                
                qDebug() << "MainWidget size:" << mainWidget->size();
                qDebug() << "PlaylistHistory position:" << windowX << windowY << "size:" << 400 << windowHeight;
                
                playlistHistory->setGeometry(windowX, windowY, 400, windowHeight);
                playlistHistory->show();
                playlistHistory->raise();  // 提升到最上层
                
                // 通过查找 topWidget 并提升它，确保顶部控件在播放列表之上
                QWidget* topWidget = mainWidget->findChild<QWidget*>();
                if (topWidget && topWidget->y() == 0 && topWidget->height() == 60) {
                    topWidget->raise();
                }
            }
        } else {
            playlistHistory->hide();
        }
    });
    
    // 连接播放历史列表的信
    connect(playlistHistory, &PlaylistHistoryQml::playRequested, this, [this](const QString& filePath){
        qDebug() << "Play from history:" << filePath;
        _play_click(filePath);
    });
    
    connect(playlistHistory, &PlaylistHistoryQml::removeRequested, this, [this](const QString& filePath){
        qDebug() << "[MVVM-UI] Remove from history:" << filePath;
        
        // 将filePath转换为QUrl
        QUrl url = QUrl::fromUserInput(filePath);
        
        // 在播放历史中查找该URL的索
        int index = AudioService::instance().playlist().indexOf(url);
        if (index >= 0) {
            qDebug() << "Removing from AudioService playlist at index:" << index;
            AudioService::instance().removeFromPlaylist(index);
        } else {
            qDebug() << "URL not found in playlist:" << url.toString();
        }
    });
    
    connect(playlistHistory, &PlaylistHistoryQml::clearAllRequested, this, [this](){
        qDebug() << "[MVVM-UI] Clear all history";
        AudioService::instance().clearPlaylist();
    });
    
    connect(playlistHistory, &PlaylistHistoryQml::pauseToggled, this, [this](){
        qDebug() << "[MVVM-UI] Pause toggled from playlist history";
        m_playbackViewModel->togglePlayPause();
        // 状态会通过isPlayingChanged信号自动同步，无需手动更新
    });
    
    // ========== 播放模式控制 ==========
    connect(process_slider, &ProcessSliderQml::signal_playModeChanged, this, [this](int mode){
        qDebug() << "[MVVM-UI] Play mode changed to:" << mode;
        // 0: Sequential, 1: RepeatOne, 2: RepeatAll, 3: Shuffle
        AudioService::PlayMode playMode = static_cast<AudioService::PlayMode>(mode);
        AudioService::instance().setPlayMode(playMode);
    });

    // ========== 旧架构进度更新和 seek 连接（已注释==========
    /*
    connect(take_pcm.get(), &TakePcm::signal_move, this, [this]() {
        // seek 完成后重新连durations（参考原 ProcessSlider 实现
        qDebug() << "TakePcm::signal_move - 重新连接 durations";
        durationsConnection = connect(work.get(), &Worker::durations, this, [this](qint64 milliseconds){
            int seconds = static_cast<int>(milliseconds / 1000);
            process_slider->setCurrentSeconds(seconds);
        });
        work->slot_setMove();
     });
    connect(take_pcm.get(), &TakePcm::signal_worker_play, work.get(), &Worker::Pause);
    connect(this,&PlayWidget::signal_set_SliderMove,work.get(),&Worker::set_SliderMove);
    
    // 更新进度条当前时间显
    // Worker::durations 发出的单位是毫秒
    durationsConnection = connect(work.get(), &Worker::durations, this, [this](qint64 milliseconds){
        int seconds = static_cast<int>(milliseconds / 1000);
        process_slider->setCurrentSeconds(seconds);
        work->slot_setMove();
    });

    // 处理滑块按下和松开事件（参考原 ProcessSlider 实现
    connect(process_slider, &ProcessSliderQml::signal_sliderPressed, [this](){
        qDebug() << "拖动进度- 断开 durations 连接";
        // 断开 Worker::durations 连接，避免拖动时进度跳动
        disconnect(durationsConnection);
    });

    // 注意：不sliderReleased 时重连，而是TakePcm::signal_move 时重
    // 这样可以避免 seek 期间的时间戳更新干扰
    */
    
    // ========== 新架构进度更新（通过ViewModel==========
    // 注意：positionChanged信号频率很高，已在前面连接到ViewModel
    
    // 处理滑块拖动 - 暂时断开位置更新
    connect(process_slider, &ProcessSliderQml::signal_sliderPressed, [this](){
        qDebug() << "[MVVM-UI] Slider pressed - temporarily disconnecting position updates";
        // ViewModel的position信号会继续发出，但UI会忽略（通过process_slider内部isSliderPressed控制
    });
    
    // Seek完成 - 重新开始接收位置更
    connect(process_slider, &ProcessSliderQml::signal_sliderReleased, [this](){
        qDebug() << "[MVVM-UI] Slider released - reconnecting position updates";
        // 不需要手动重连，ViewModel的position信号一直在
    });

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
    
    // 控制歌词显示状
    if (lyricDisplay) {
        if (flag) {
            qDebug() << "Showing lyric display";
            lyricDisplay->show();  // 展开时显示歌
            lyricDisplay->setIsUp(true);
            
            // 立即更新歌词高亮行到当前播放位置
            auto session = AudioService::instance().currentSession();
            if (session) {
                AudioPlayer& player = AudioPlayer::instance();
                // 使用播放位置而不是解码时间戳（seek后解码时间戳会滞后）
                qint64 currentPosition = player.getPlaybackPosition();
                int timeInMs = static_cast<int>(currentPosition) + 1000;  // 歌词提前1000ms
                
                int targetLine = -1;
                for (auto it = lyrics.begin(); it != lyrics.end(); ++it) {
                    if (it->first <= timeInMs) {
                        auto next = std::next(it);
                        if (next == lyrics.end() || next->first > timeInMs) {
                            targetLine = std::distance(lyrics.begin(), it) + 5;
                            break;
                        }
                    }
                }
                
                if (targetLine >= 0) {
                    lyricDisplay->highlightLine(targetLine);
                    lyricDisplay->scrollToLine(targetLine);
                    lyricDisplay->currentLine = targetLine;
                    qDebug() << "Initial lyric highlight set to line:" << targetLine << "at position:" << currentPosition << "ms";
                }
            }
        } else {
            qDebug() << "Hiding lyric display";
            lyricDisplay->hide();  // 收起时隐藏歌
            lyricDisplay->setIsUp(false);
        }
    }
    
    // 触发重绘以更新背
    update();
    
    emit signal_isUpChanged(isUp);
}
void PlayWidget::slot_work_stop(){
    qDebug() << __FUNCTION__ << "暂停";
    emit signal_playState(ProcessSliderQml::Pause);
    if(filePath.size())
    {
        emit signal_stop_rotate(false);
        qDebug() << "[PLAY_STATE] slot_work_stop 发出 signal_play_button_click(false," << filePath << ")";
        
        // 延迟UI列表更新，避免阻
        QString currentFilePath = filePath;
        QTimer::singleShot(0, this, [this, currentFilePath]() {
            emit signal_play_button_click(false, currentFilePath);
        });
        
        process_slider->setSongName(QFileInfo(fileName).baseName());
        nameLabel->setText(QFileInfo(fileName).baseName());
    }
}
void PlayWidget::slot_work_play(){
    emit signal_playState(ProcessSliderQml::Play);
    emit signal_stop_rotate(true);
    qDebug() << "[PLAY_STATE] slot_work_play 发出 signal_play_button_click(true," << filePath << ")";
    
    // 延迟100ms更新UI列表，确保音频完全启动后再更
    QString currentFilePath = filePath;
    QTimer::singleShot(100, this, [this, currentFilePath]() {
        emit signal_play_button_click(true, currentFilePath);
    });
    
    process_slider->setSongName(QFileInfo(fileName).baseName());
    nameLabel->setText(QFileInfo(fileName).baseName());
}
void PlayWidget::slot_Lrc_send_lrc(const std::map<int, std::string> lyrics){
    this->lyricDisplay->currentLine = 5;  // 初始行设，对应第一行歌
    this->lyrics.clear();
    
    {
        std::lock_guard<std::mutex>lock(mtx);
        this->lyrics = lyrics;
    }
    
    // 设置歌曲信息到歌词显示界
    if (!fileName.isEmpty()) {
        QFileInfo fileInfo(fileName);
        QString songName = fileInfo.baseName();
        lyricDisplay->setSongInfo(songName, ""); // 暂时没有艺术家信
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
            // 如果第一句是空的，找到第一句非空歌
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
    
    // 设置歌曲名字到桌面歌
    if (!fileName.isEmpty()) {
        QFileInfo fileInfo(fileName);
        QString songName = fileInfo.baseName();
        qDebug() << "Setting desktop song name:" << songName;
        desk->setSongName(songName);
    }
}
void PlayWidget::slot_play_click(){
    qDebug() << "[TIMING] slot_play_click START" << QTime::currentTime().toString("hh:mm:ss.zzz");
    
    // ========== MVVM架构：通过ViewModel切换播放/暂停 ==========
    qDebug() << "[MVVM-UI] slot_play_click: Using ViewModel togglePlayPause";
    
    ProcessSliderQml::State currentState = controlBar->getState();
    
    // 检查是否播放完成后首次点击（进度在开头且状态为 Stop
    QQuickItem* root = controlBar->rootObject();
    if (root && currentState == ProcessSliderQml::Stop) {
        QVariant currentTime = root->property("currentTime");
        if (currentTime.isValid() && currentTime.toInt() == 0) {
            qDebug() << "[MVVM-UI] Restart playback after completion";
            // 使用ViewModel重新播放
            if (!m_playbackViewModel->currentFilePath().isEmpty()) {
                QUrl url = m_playbackViewModel->currentFilePath().startsWith("http", Qt::CaseInsensitive) 
                    ? QUrl(m_playbackViewModel->currentFilePath()) 
                    : QUrl::fromLocalFile(m_playbackViewModel->currentFilePath());
                m_playbackViewModel->play(url);
            }
            qDebug() << "[TIMING] slot_play_click END" << QTime::currentTime().toString("hh:mm:ss.zzz");
            return;
        }
    }
    
    // 切换播放/暂停状
    m_playbackViewModel->togglePlayPause();
    
    // UI状态会自动通过ViewModel的isPlayingChanged信号更新
    
    qDebug() << "[TIMING] slot_play_click END" << QTime::currentTime().toString("hh:mm:ss.zzz");
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

    // 打开文件对话
    QString filePath_ = QFileDialog::getOpenFileName(
                &dummyWidget,                   // 父窗口（可以nullptr
                QStringLiteral(u"\u9009\u62e9\u97f3\u9891\u6587\u4ef6"), // 对话框标题
                QDir::homePath(),               // 起始目录（可以是任意路径
                QStringLiteral(u"\u97f3\u9891\u6587\u4ef6 (*.mp3 *.wav *.flac *.ogg *mp4);;\u6240\u6709\u6587\u4ef6 (*)") // 文件过滤
                );

    // 打印选中的文件路
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
        
        // ========== MVVM架构：通过ViewModel播放 ==========
        qDebug() << "[MVVM-UI] _play_click: Playing via ViewModel:" << songPath;
        
        // 使用ViewModel播放
        QUrl url;
        if (songPath.startsWith("http", Qt::CaseInsensitive)) {
            url = QUrl(songPath);  // 网络URL
        } else {
            url = QUrl::fromLocalFile(songPath);  // 本地文件
        }
        
        m_playbackViewModel->play(url);
        
        // ViewModel会自动：
        // 1. 调用AudioService播放
        // 2. 更新内部状态（isPlaying, position, duration等）
        // 3. 发出shouldLoadLyrics信号触发歌词加载
        // 4. 发出shouldStartRotation信号触发旋转动画
        
        // 这里只需要处理UI层特有的逻辑
        emit signal_begin_to_play(songPath);
    }
    else
    {
        // 相同路径，切换播暂停
        slot_play_click();
    }
}
void PlayWidget::_remove_click(QString songName)
{
    if(songName == this->filePath)
    {
        this->fileName.clear();
        this->filePath.clear();

        emit signal_remove_click();
    }
}
void PlayWidget::setPianWidgetEnable(bool flag)
{
    // PianWidget 现在集成ProcessSliderQml 中，不需要单独控
}
bool PlayWidget::checkAndWarnIfPathNotExists(const QString &path) {

    if (path.startsWith("http", Qt::CaseInsensitive)) {
        qDebug() << "检测到网络路径，跳过存在性检" << path;
        return true;
    }

    QFileInfo fileInfo(path);

    if (fileInfo.exists()) {
        return true;
    } else {
        QMessageBox::warning(nullptr,
                             QStringLiteral(u"\u8def\u5f84\u4e0d\u5b58\u5728"),
                             QStringLiteral(u"\u6587\u4ef6\u4e0d\u5b58\u5728\u6216\u65e0\u6cd5\u8bbf\u95ee\uff1a\n%1").arg(path));
        return false;
    }
}
PlayWidget::~PlayWidget()
{
    qDebug() << "PlayWidget::~PlayWidget() - Starting cleanup...";
 
    // ========== 旧架构清理（已注释，保留供参考） ==========
    /*
    // 2. 等待线程完成当前工作
    QThread::msleep(200);
    
    // 3. 退出并等待所QThread
    if(a)
    {
        qDebug() << "PlayWidget: Stopping thread a...";
        a->quit();
        if(!a->wait(2000)) {  // 等待最
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
    
    // 4. 清理 shared_ptr（会自动调用析构函数
    work.reset();
    lrc.reset();
    take_pcm.reset();
    */
    
    // ========== 新架构清==========
    // 停止当前会话
    if (currentSession) {
        currentSession->stop();
        currentSession = nullptr;
    }

    // Ensure QQuickWidget-based components are torn down while GUI/OpenGL
    // context is still valid to avoid shutdown-time render-control asserts.
    auto shutdownQuickWidget = [](QQuickWidget* widget) {
        if (!widget) {
            return;
        }
        widget->hide();
        widget->setSource(QUrl());
    };

    shutdownQuickWidget(desk);
    shutdownQuickWidget(lyricDisplay);
    shutdownQuickWidget(process_slider);
    shutdownQuickWidget(playlistHistory);
    
    // 清理歌词线程（保留）
    if(b)
    {
        qDebug() << "PlayWidget: Stopping lyric thread b...";
        b->quit();
        if(!b->wait(2000)) {
            qDebug() << "PlayWidget: Thread b did not finish, terminating...";
            b->terminate();
            b->wait();
        }
    }
    
    if (desk) {
        delete desk;
        desk = nullptr;
    }

    if (lyricDisplay) {
        delete lyricDisplay;
        lyricDisplay = nullptr;
    }

    if (process_slider) {
        delete process_slider;
        process_slider = nullptr;
        controlBar = nullptr;
    }

    if (playlistHistory) {
        delete playlistHistory;
        playlistHistory = nullptr;
    }

    lrc.reset();
    
    qDebug() << "PlayWidget::~PlayWidget() - Cleanup complete";
}

void PlayWidget::slot_updateBackground(QString picPath) {
    qDebug() << "slot_updateBackground called with:" << picPath;
    
    // 如果file:/// URL 格式，转换为本地路径
    QString localPath = picPath;
    if (picPath.startsWith("file:///")) {
        localPath = QUrl(picPath).toLocalFile();
        qDebug() << "Converted URL to local path:" << localPath;
    } else if (picPath.startsWith("qrc:")) {
        // QRC资源路径，去掉qrc:前缀，保/
        localPath = picPath.mid(3);  // 去掉"qrc"，保:/..."
        qDebug() << "Converted QRC path:" << localPath;
    }
    
    // 加载原始图片
    QPixmap originalPixmap(localPath);
    if (originalPixmap.isNull()) {
        qDebug() << "Failed to load album cover for background:" << localPath;
        // 使用默认背景图片
        qDebug() << "Using default background image";
        originalPixmap.load(":/new/prefix1/icon/Music.png");
        if (originalPixmap.isNull()) {
            qDebug() << "Failed to load default background image, skipping";
            return;
        }
    }
    
    qDebug() << "Original pixmap size:" << originalPixmap.size();
    
    // 1. 缩放到合适大小（1000x600），保持比例并填
    QPixmap scaledPixmap = originalPixmap.scaled(1000, 600, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    
    // 2. 裁剪到中心区
    int x = (scaledPixmap.width() - 1000) / 2;
    int y = (scaledPixmap.height() - 600) / 2;
    QPixmap croppedPixmap = scaledPixmap.copy(x, y, 1000, 600);
    
    qDebug() << "Cropped pixmap size:" << croppedPixmap.size();
    
    // 3. 创建强模糊效果的图片 - 多次缩放模拟高斯模糊
    QImage image = croppedPixmap.toImage();
    
    // 使用更小的尺寸实现更强的模糊效果
    // 第一次模糊：缩小100x60
    QImage smallImage1 = image.scaled(100, 60, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    // 第二次模糊：放大200x120
    QImage smallImage2 = smallImage1.scaled(200, 120, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    // 第三次模糊：放大到最终尺
    QImage blurredImage = smallImage2.scaled(1000, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    
    // 4. 添加更深的半透明深色遮罩，增强对比度和歌词可读
    QPainter maskPainter(&blurredImage);
    maskPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    maskPainter.fillRect(blurredImage.rect(), QColor(0, 0, 0, 140));  // 增加遮罩透明度从 120 140
    maskPainter.end();
    
    // 5. 转换QPixmap 并设
    QPixmap blurredPixmap = QPixmap::fromImage(blurredImage);
    backgroundLabel->setPixmap(blurredPixmap);
    
    // 触发重绘，让 paintEvent 使用新的背景
    update();
    
    qDebug() << "Background updated successfully, pixmap size:" << blurredPixmap.size();
    qDebug() << "backgroundLabel visible:" << backgroundLabel->isVisible() << "geometry:" << backgroundLabel->geometry();
}

void PlayWidget::slot_lyric_seek(int timeMs) {
    qDebug() << "[MVVM-UI] Seeking to time:" << timeMs << "ms";
    
    // 使用ViewModel进行跳转
    m_playbackViewModel->seekTo(timeMs);
    lastSeekPosition = timeMs;
    
    // 同步更新进度条显示位
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
    
    // 断开歌词更新信号，避免拖拽时的冲
    if (lyricUpdateConnection) {
        disconnect(lyricUpdateConnection);
    }
}

void PlayWidget::slot_lyric_drag_end() {
    qDebug() << "Lyric drag ended - reconnecting lyric update";
    
    // ========== 旧架构重连（已注释） ==========
    /*
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
    */
    
    // ========== 新架构重==========
    lyricUpdateConnection = connect(&AudioService::instance(), &AudioService::positionChanged, 
            this, [this](qint64 positionMs) {
        int targetLine = -1;
        int timeInMs = static_cast<int>(positionMs);
        
        for (auto it = lyrics.begin(); it != lyrics.end(); ++it) {
            if (it->first <= timeInMs) {
                auto next = std::next(it);
                if (next == lyrics.end() || next->first > timeInMs) {
                    targetLine = std::distance(lyrics.begin(), it) + 5;
                    break;
                }
            }
        }
        
        if (targetLine >= 0 && targetLine != lyricDisplay->currentLine) {
            lyricDisplay->highlightLine(targetLine);
            lyricDisplay->scrollToLine(targetLine);
            lyricDisplay->currentLine = targetLine;
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
    
    // 更新进度条的预览位置，但不实际跳
    if (process_slider) {
        QQuickItem* root = process_slider->rootObject();
        if (root) {
            QVariant totalDuration = root->property("totalDuration");
            if (totalDuration.isValid() && totalDuration.toInt() > 0) {
                double previewRatio = static_cast<double>(timeMs) / totalDuration.toInt();
                // 只更新视觉位置，不触发跳
                root->setProperty("previewValue", previewRatio);
            }
        }
    }
}

PlaybackViewModel* PlayWidget::playbackViewModel() const
{
    return m_playbackViewModel;
}

bool PlayWidget::get_net_flag()
{
    return play_net;
}

void PlayWidget::set_play_net(bool flag)
{
    play_net = flag;
    emit signal_netFlagChanged(flag);
}

void PlayWidget::setNetworkMetadata(const QString& artist, const QString& cover)
{
    networkSongArtist = artist;
    networkSongCover = cover;
}

void PlayWidget::setNetworkMetadata(const QString& title, const QString& artist, const QString& cover)
{
    currentSongTitle = title;
    currentSongArtist = artist;
    networkSongArtist = artist;
    networkSongCover = cover;
}
