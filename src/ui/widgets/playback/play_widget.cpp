#include "play_widget.h"
#include <QTime>
#include <QGraphicsBlurEffect>
#include <QPainter>
#include <QQuickWidget>
#include <QUrl>

namespace {

QString decodedFileNameFromPath(const QString& songPath)
{
    if (songPath.trimmed().isEmpty()) {
        return QString();
    }

    if (songPath.startsWith("http", Qt::CaseInsensitive)) {
        const QUrl url(songPath);
        QString decodedPath = QUrl::fromPercentEncoding(url.path().toUtf8());
        decodedPath = decodedPath.trimmed();
        if (!decodedPath.isEmpty()) {
            const QFileInfo info(decodedPath);
            const QString name = info.fileName().trimmed();
            if (!name.isEmpty()) {
                return name;
            }
        }
    }

    const QFileInfo fallbackInfo(songPath);
    return QUrl::fromPercentEncoding(fallbackInfo.fileName().toUtf8()).trimmed();
}

QString displayTitleFromFileName(const QString& fileName)
{
    const QString decoded = QUrl::fromPercentEncoding(fileName.toUtf8()).trimmed();
    if (decoded.isEmpty()) {
        return QString();
    }
    const QFileInfo info(decoded);
    QString title = info.completeBaseName().trimmed();
    if (title.isEmpty()) {
        title = info.baseName().trimmed();
    }
    if (title.isEmpty()) {
        title = decoded;
    }
    return title;
}

}

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
    process_slider->setMinimumHeight(72);
    process_slider->setMaxSeconds(0);
    process_slider->setVolume(m_playbackViewModel->volume());
    
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
    connect(music, &QPushButton::clicked, this, &PlayWidget::handleMusicButtonClicked);
    music->move(10, 10);

    // ========== 新架构初始化 ==========
    // 创建歌词解析线程（保留）
    b = new QThread();
    lrc = std::make_shared<LrcAnalyze>();
    // lrc 不需要移到线程，或者可以根据需要决
    
    // 初始化音频服务（单例
    qDebug() << "[MVVM-UI] AudioService initialized";
    
    initLyricDisplay();

    rotatingCircleHost = new QWidget(this);
    rotatingCircleHost->setAttribute(Qt::WA_TranslucentBackground, true);
    rotatingCircleHost->setAutoFillBackground(false);
    rotatingCircleHost->setStyleSheet("background: transparent;");
    rotatingCircle = new RotatingCircleImage(rotatingCircleHost);
    rotatingCircle->setAttribute(Qt::WA_TranslucentBackground, true);
    rotatingCircle->setAutoFillBackground(false);
    rotatingCircle->setStyleSheet("background: transparent;");
    rotatingCircle->resize(300, 300);
    
    // ========== MVVM架构：连接ViewModel信号到UI ==========
    setupPlaybackViewModelConnections();

    nameLabel = new QLabel(this);
    nameLabel->setAttribute(Qt::WA_TranslucentBackground, true);
    nameLabel->setStyleSheet("QLabel { color: white; font-size: 28px; background: transparent; }");
    nameLabel->setWordWrap(true);
    nameLabel->setMinimumHeight(30);
    nameLabel->setAlignment(Qt::AlignCenter);

    // ========== 旧架构状态连接（已注释） ==========
    /*
    connect(work.get(),&Worker::Stop,this, &PlayWidget::onWorkStop);
    connect(work.get(),&Worker::Begin,this, &PlayWidget::onWorkPlay);
    */
    
    // ========== 新架构状态更新已在前面通过 audioService 信号处理 ==========
    setupCoreConnections();

    setupControlAndPlaylistConnections();

    updateAdaptiveLayout();
}

void PlayWidget::handleMusicButtonClicked()
{
    // 切换状态而不是总是收起。
    emit signalBigClicked(!isUp);
}

void PlayWidget::handleBufferingStateChanged(bool active)
{
    qDebug() << "[MVVM-UI] Buffering state changed:" << active;
    if (!nameLabel) {
        return;
    }

    if (active) {
        qDebug() << "[MVVM-UI] Buffering started";
        nameLabel->setText("正在缓冲...");
        return;
    }

    qDebug() << "[MVVM-UI] Buffering finished";
    const QString title = !currentSongTitle.trimmed().isEmpty()
            ? currentSongTitle.trimmed()
            : displayTitleFromFileName(fileName);
    if (!title.isEmpty()) {
        nameLabel->setText(title);
    }
}

void PlayWidget::handleProcessChangeRequested(qint64 milliseconds, bool back_flag)
{
    Q_UNUSED(back_flag);
    qDebug() << "Seeking to position:" << milliseconds << "ms";
    if (!m_playbackViewModel) {
        qDebug() << "No active ViewModel to seek";
        return;
    }
    m_playbackViewModel->seekTo(milliseconds);
    lastSeekPosition = milliseconds;
}

void PlayWidget::handleDeferredSeekAfterPlay()
{
    if (!m_playbackViewModel || pendingSeekPositionMs < 0) {
        return;
    }
    m_playbackViewModel->seekTo(pendingSeekPositionMs);
    pendingSeekPositionMs = -1;
}

void PlayWidget::handleSliderMoveRequested(int seconds)
{
    qDebug() << "[MVVM-UI] Slider moved to:" << seconds << "seconds";
    const qint64 milliseconds = static_cast<qint64>(seconds) * 1000;

    // 用户拖动音频进度条即表示切回音频焦点并进入播放态。
    emit signalPlayState(ProcessSliderQml::Play);

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
            pendingSeekPositionMs = milliseconds;
            m_playbackViewModel->play(resumeUrl);
            QTimer::singleShot(0, this, &PlayWidget::handleDeferredSeekAfterPlay);
        } else {
            // 回退：没有可恢复曲目时仅执行定位。
            m_playbackViewModel->seekTo(milliseconds);
        }
    }
}

void PlayWidget::handleCoverExpandRequested()
{
    qDebug() << "[PlayWidget] signalUpClick received, isUp =" << isUp;
    // 底部封面区点击仅负责“进入歌词页”，避免在已展开态被误触后直接收起。
    if (!isUp) {
        emit signalBigClicked(true);
    }
}

void PlayWidget::handleLyricPositionChanged()
{
    const qint64 positionMs = m_playbackViewModel ? m_playbackViewModel->position() : 0;
    // 根据时间找到对应的歌词行。
    int targetLine = -1;
    // 歌词提前1000ms显示（让歌词切换与播放进度更同步）。
    const int timeInMs = static_cast<int>(positionMs) + 1000;

    for (auto it = lyrics.begin(); it != lyrics.end(); ++it) {
        if (it->first <= timeInMs) {
            const auto next = std::next(it);
            if (next == lyrics.end() || next->first > timeInMs) {
                // +5 对应初始偏移。
                targetLine = std::distance(lyrics.begin(), it) + 5;
                break;
            }
        }
    }

    if (targetLine >= 0 && lyricDisplay && targetLine != lyricDisplay->currentLine) {
        lyricDisplay->highlightLine(targetLine);
        lyricDisplay->scrollToLine(targetLine);
        lyricDisplay->currentLine = targetLine;
        update();
    }
}

void PlayWidget::handleSimilarPlayRequested(const QVariantMap& item)
{
    emit signalSimilarSongSelected(item);
}

void PlayWidget::queuePlayButtonStateUpdate(bool playing, const QString& path)
{
    m_pendingPlayButtonStateValid = true;
    m_pendingPlayButtonPlaying = playing;
    m_pendingPlayButtonPath = path;
    QTimer::singleShot(0, this, &PlayWidget::handleDeferredPlayButtonStateUpdate);
}

void PlayWidget::handleDeferredPlayButtonStateUpdate()
{
    if (!m_pendingPlayButtonStateValid) {
        return;
    }

    const bool playing = m_pendingPlayButtonPlaying;
    const QString path = m_pendingPlayButtonPath;
    m_pendingPlayButtonStateValid = false;
    m_pendingPlayButtonPath.clear();
    emit signalPlayButtonClick(playing, path);
}

void PlayWidget::shutdownQuickWidget(QQuickWidget* widget)
{
    if (!widget) {
        return;
    }
    widget->hide();
    widget->setSource(QUrl());
}

void PlayWidget::onDeskToggled(bool checked){
    if(checked){
        desk->show();
    }else{
        desk->hide();
    }
}
void PlayWidget::setIsUp(bool flag){
    isUp = flag;
    
    qDebug() << "PlayWidget::setIsUp called with flag:" << flag;
    qDebug() << "Call stack trace - isUp:" << flag;

    // 展开时如果还没有封面背景，使用深色兜底，避免出现白底。
    if (flag && backgroundLabel && (!backgroundLabel->pixmap() || backgroundLabel->pixmap()->isNull())) {
        QPixmap fallback(qMax(width(), 1), qMax(height(), 1));
        fallback.fill(Qt::transparent);
        QPainter painter(&fallback);
        QLinearGradient gradient(0, 0, fallback.width(), fallback.height());
        gradient.setColorAt(0.0, QColor("#0D1218"));
        gradient.setColorAt(0.5, QColor("#151B24"));
        gradient.setColorAt(1.0, QColor("#10151D"));
        painter.fillRect(fallback.rect(), gradient);
        backgroundLabel->setPixmap(fallback);
    }
    
    // 控制歌词显示状
    if (lyricDisplay) {
        if (flag) {
            qDebug() << "Showing lyric display";
            lyricDisplay->show();  // 展开时显示歌
            lyricDisplay->setIsUp(true);
            
            // 立即更新歌词高亮行到当前播放位置
            const qint64 currentPosition = m_playbackViewModel ? m_playbackViewModel->position() : 0;
            if (currentPosition >= 0) {
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

    updateAdaptiveLayout();
    
    emit signalIsUpChanged(isUp);
}

int PlayWidget::collapsedPlaybackHeight() const
{
    return process_slider ? process_slider->height() : 90;
}

void PlayWidget::updateAdaptiveLayout()
{
    const int wWidth = qMax(1, width());
    const int wHeight = qMax(1, height());

    const int controlHeight = qBound(72, wHeight / 8, 108);
    if (process_slider) {
        process_slider->setGeometry(0, wHeight - controlHeight, wWidth, controlHeight);
    }

    if (music) {
        music->move(10, 10);
    }

    const int topMargin = 20;
    const int contentTop = topMargin + 50;
    const int contentBottom = wHeight - controlHeight - 12;
    const int contentHeight = qMax(120, contentBottom - contentTop);

    const int circleDiameter = qBound(180, qMin(wWidth, contentHeight) / 2, 380);
    if (rotatingCircleHost) {
        const int circleX = qMax(24, wWidth / 10);
        const int circleY = contentTop + qMax(0, (contentHeight - circleDiameter) / 2);
        rotatingCircleHost->setGeometry(circleX, circleY, circleDiameter, circleDiameter);
    }
    if (rotatingCircle) {
        rotatingCircle->setGeometry(0, 0, rotatingCircleHost ? rotatingCircleHost->width() : circleDiameter,
                                    rotatingCircleHost ? rotatingCircleHost->height() : circleDiameter);
    }

    if (nameLabel) {
        // 歌词页标题始终以窗口为基准水平居中，避免缩放后偏向右侧区域。
        const int nameSideMargin = qBound(18, wWidth / 20, 64);
        const int nameWidth = qMax(260, wWidth - nameSideMargin * 2);
        const int nameX = (wWidth - nameWidth) / 2;
        nameLabel->setGeometry(nameX, 44, nameWidth, 36);
    }

    if (lyricDisplay) {
        const int lyricX = qBound(220, wWidth / 3, wWidth - 260);
        const int lyricY = contentTop;
        const int lyricWidth = qMax(260, wWidth - lyricX - 18);
        const int lyricHeight = qMax(160, contentBottom - lyricY);
        lyricDisplay->setGeometry(lyricX, lyricY, lyricWidth, lyricHeight);

        // 高亮歌词的目标中心按“窗口可用播放区（去掉底部控制栏）”计算，
        // 而不是按 LyricDisplay 自身区域中点，避免全屏后视觉中心偏下。
        const double playbackAreaCenterY = (wHeight - controlHeight) / 2.0;
        const double localCenterY = playbackAreaCenterY - lyricY;
        const double centerOffsetY = localCenterY - lyricHeight / 2.0;
        lyricDisplay->setCenterYOffset(centerOffsetY);
    }

    if (isUp) {
        clearMask();
    } else {
        setMask(QRegion(0, wHeight - controlHeight, wWidth, controlHeight));
    }
}

void PlayWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateAdaptiveLayout();
}

void PlayWidget::onWorkStop(){
    qDebug() << __FUNCTION__ << "暂停";
    emit signalPlayState(ProcessSliderQml::Pause);
    if(filePath.size())
    {
        emit signalStopRotate(false);
        qDebug() << "[PLAY_STATE] onWorkStop 发出 signalPlayButtonClick(false," << filePath << ")";
        
        // 延迟 UI 列表更新，避免阻塞主播放链路。
        queuePlayButtonStateUpdate(false, filePath);
        
        const QString title = !currentSongTitle.trimmed().isEmpty()
                ? currentSongTitle.trimmed()
                : displayTitleFromFileName(fileName);
        process_slider->setSongName(title);
        nameLabel->setText(title);
    }
}
void PlayWidget::onWorkPlay(){
    emit signalPlayState(ProcessSliderQml::Play);
    emit signalStopRotate(true);
    qDebug() << "[PLAY_STATE] onWorkPlay 发出 signalPlayButtonClick(true," << filePath << ")";
    
    // 延迟更新 UI 列表，确保音频完全启动后再更新状态。
    queuePlayButtonStateUpdate(true, filePath);
    
    const QString title = !currentSongTitle.trimmed().isEmpty()
            ? currentSongTitle.trimmed()
            : displayTitleFromFileName(fileName);
    process_slider->setSongName(title);
    nameLabel->setText(title);
}
void PlayWidget::onLrcSendLrc(const std::map<int, std::string> lyrics){
    this->lyricDisplay->currentLine = 5;  // 初始行设，对应第一行歌
    this->lyrics.clear();
    
    {
        std::lock_guard<std::mutex>lock(mtx);
        this->lyrics = lyrics;
    }
    
    // 设置歌曲信息到歌词显示界
    if (!fileName.isEmpty() || !currentSongTitle.trimmed().isEmpty()) {
        const QString songName = !currentSongTitle.trimmed().isEmpty()
                ? currentSongTitle.trimmed()
                : displayTitleFromFileName(fileName);
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
    if (!fileName.isEmpty() || !currentSongTitle.trimmed().isEmpty()) {
        const QString songName = !currentSongTitle.trimmed().isEmpty()
                ? currentSongTitle.trimmed()
                : displayTitleFromFileName(fileName);
        qDebug() << "Setting desktop song name:" << songName;
        desk->setSongName(songName);
    }
}
void PlayWidget::onPlayClick(){
    qDebug() << "[TIMING] onPlayClick START" << QTime::currentTime().toString("hh:mm:ss.zzz");
    
    // ========== MVVM架构：通过ViewModel切换播放/暂停 ==========
    qDebug() << "[MVVM-UI] onPlayClick: Using ViewModel togglePlayPause";
    
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
            qDebug() << "[TIMING] onPlayClick END" << QTime::currentTime().toString("hh:mm:ss.zzz");
            return;
        }
    }
    
    // 切换播放/暂停状
    m_playbackViewModel->togglePlayPause();
    
    // UI状态会自动通过ViewModel的isPlayingChanged信号更新
    
    qDebug() << "[TIMING] onPlayClick END" << QTime::currentTime().toString("hh:mm:ss.zzz");
}

void PlayWidget::rePlay(QString path)
{
    if(path.size() == 0)
        return;
    emit signalFilepath(path);
}


void PlayWidget::initLyricDisplay()
{
    qDebug() << "Initializing LyricDisplay...";
    lyricDisplay = new LyricDisplayQml(this);
    lyricDisplay->setClearColor(QColor(Qt::transparent));
    lyricDisplay->setAttribute(Qt::WA_AlwaysStackOnTop,true);
    lyricDisplay->setMinimumSize(360, 220);
    if (lyricDisplay->rootObject()) {
        lyricDisplay->rootObject()->setProperty("showSongInfo", false);
    }
    lyricDisplay->hide();  // 初始时隐藏歌词，只有展开时才显示
    qDebug() << "LyricDisplay initialized, size:" << lyricDisplay->size() << "position:" << lyricDisplay->pos();
}


void PlayWidget::beginTakeLrc(QString str)
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
        emit signalAddSong(filename,filePath_);
    }
}

void PlayWidget::playClick(QString songPath)
{
    if(songPath != this->filePath)
    {
        this->filePath = songPath;

        fileName = decodedFileNameFromPath(songPath);
        if(!checkAndWarnIfPathNotExists(songPath))
            return;
        
        // ========== MVVM架构：通过ViewModel播放 ==========
        qDebug() << "[MVVM-UI] playClick: Playing via ViewModel:" << songPath;
        
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
        emit signalBeginToPlay(songPath);
    }
    else
    {
        // 相同路径，切换播暂停
        onPlayClick();
    }
}
void PlayWidget::removeClick(QString songName)
{
    if(songName == this->filePath)
    {
        this->fileName.clear();
        this->filePath.clear();

        emit signalRemoveClick();
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

    // 先销毁 QQuickWidget 内容，避免退出阶段触发渲染上下文断言。
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

void PlayWidget::onUpdateBackground(QString picPath) {
    qDebug() << "onUpdateBackground called with:" << picPath;
    
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
    
    const int targetWidth = qMax(640, width());
    const int targetHeight = qMax(360, height());

    // 1. 按当前窗口尺寸缩放并填满
    QPixmap scaledPixmap = originalPixmap.scaled(targetWidth, targetHeight,
                                                 Qt::KeepAspectRatioByExpanding,
                                                 Qt::SmoothTransformation);
    
    // 2. 裁剪到目标尺寸中心区域
    int x = (scaledPixmap.width() - targetWidth) / 2;
    int y = (scaledPixmap.height() - targetHeight) / 2;
    QPixmap croppedPixmap = scaledPixmap.copy(x, y, targetWidth, targetHeight);
    
    qDebug() << "Cropped pixmap size:" << croppedPixmap.size();
    
    // 3. 创建强模糊效果的图片 - 多次缩放模拟高斯模糊
    QImage image = croppedPixmap.toImage();
    
    // 使用分段缩放生成模糊背景，分辨率随窗口尺寸自适应
    const int blurW1 = qMax(96, targetWidth / 10);
    const int blurH1 = qMax(54, targetHeight / 10);
    const int blurW2 = qMax(192, targetWidth / 5);
    const int blurH2 = qMax(108, targetHeight / 5);
    QImage smallImage1 = image.scaled(blurW1, blurH1, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QImage smallImage2 = smallImage1.scaled(blurW2, blurH2, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QImage blurredImage = smallImage2.scaled(targetWidth, targetHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    
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

void PlayWidget::onLyricSeek(int timeMs) {
    qDebug() << "[MVVM-UI] Seeking to time:" << timeMs << "ms";

    if (process_slider) {
        process_slider->setSeekPendingSeconds(timeMs / 1000);
    }
    
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

void PlayWidget::onLyricDragStart() {
    qDebug() << "Lyric drag started - disconnecting lyric update";
    
    // 断开歌词更新信号，避免拖拽时的冲
    if (lyricUpdateConnection) {
        disconnect(lyricUpdateConnection);
    }
}

void PlayWidget::onLyricDragEnd() {
    qDebug() << "Lyric drag ended - reconnecting lyric update";

    // ========== 新架构重==========
    lyricUpdateConnection = connect(m_playbackViewModel, &PlaybackViewModel::positionChanged,
                                    this, &PlayWidget::handleLyricPositionChanged);
}

void PlayWidget::onLyricPreview(int timeMs) {
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
                const int maxSeconds = totalDuration.toInt();
                int previewSeconds = timeMs / 1000;
                if (previewSeconds < 0) {
                    previewSeconds = 0;
                }
                if (previewSeconds > maxSeconds) {
                    previewSeconds = maxSeconds;
                }
                // 只更新视觉显示，不触发实际 seek。
                root->setProperty("currentTime", previewSeconds);
            }
        }
    }
}

void PlayWidget::setSimilarRecommendations(const QVariantList& items)
{
    if (lyricDisplay) {
        lyricDisplay->setSimilarSongs(items);
    }
}

void PlayWidget::clearSimilarRecommendations()
{
    if (lyricDisplay) {
        lyricDisplay->clearSimilarSongs();
    }
}

PlaybackViewModel* PlayWidget::playbackViewModel() const
{
    return m_playbackViewModel;
}

bool PlayWidget::getNetFlag()
{
    return play_net;
}

void PlayWidget::setPlayNet(bool flag)
{
    play_net = flag;
    emit signalNetFlagChanged(flag);
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
