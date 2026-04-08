#include "play_widget.h"

#include "settings_manager.h"

#include <QDir>
#include <QGraphicsBlurEffect>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QQuickWidget>
#include <QTime>
#include <QUrl>

namespace {

QString decodedFileNameFromPath(const QString& songPath) {
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

QString normalizeMediaPathForPlayback(const QString& rawPath) {
    const QString trimmed = rawPath.trimmed();
    if (trimmed.isEmpty()) {
        return trimmed;
    }

    if (trimmed.startsWith("http", Qt::CaseInsensitive)) {
        const QUrl url(trimmed);
        const QString decoded = url.toString(QUrl::FullyDecoded);
        return decoded.isEmpty() ? trimmed : decoded;
    }

    return QDir::fromNativeSeparators(trimmed);
}

QString displayTitleFromFileName(const QString& fileName) {
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

QString stageSubtitleText() {
    return QString();
}

struct StageLayoutSpec {
    bool lyricsFocus = false;
    int stageTopMargin = 20;
    int stageHorizontalMargin = 52;
    int columnGap = 56;
    qreal coverColumnWidthRatio = 0.42;
    int coverMinSize = 240;
    int coverMaxSize = 500;
    int lyricColumnMaxWidth = 520;
    int lyricsPanelMinHeight = 330;
    int stackedBreakpoint = 1100;
    int stackedGap = 30;
};

StageLayoutSpec stageLayoutSpecForStyle(int styleId) {
    StageLayoutSpec spec;
    switch (styleId) {
        case 1:
            spec.stageTopMargin = 20;
            spec.stageHorizontalMargin = 52;
            spec.columnGap = 54;
            spec.coverColumnWidthRatio = 0.42;
            spec.coverMaxSize = 430;
            spec.lyricColumnMaxWidth = 500;
            spec.lyricsPanelMinHeight = 320;
            break;
        case 2:
            spec.stageTopMargin = 18;
            spec.stageHorizontalMargin = 56;
            spec.columnGap = 64;
            spec.coverColumnWidthRatio = 0.44;
            spec.coverMinSize = 300;
            spec.coverMaxSize = 560;
            spec.lyricColumnMaxWidth = 540;
            spec.lyricsPanelMinHeight = 340;
            break;
        case 3:
            spec.lyricsFocus = true;
            spec.stageTopMargin = 18;
            spec.stageHorizontalMargin = 60;
            spec.columnGap = 36;
            spec.coverMinSize = 220;
            spec.coverMaxSize = 280;
            spec.lyricColumnMaxWidth = 920;
            spec.lyricsPanelMinHeight = 360;
            spec.stackedGap = 26;
            break;
        case 4:
            spec.stageTopMargin = 16;
            spec.stageHorizontalMargin = 52;
            spec.columnGap = 56;
            spec.coverColumnWidthRatio = 0.40;
            spec.coverMinSize = 220;
            spec.coverMaxSize = 400;
            spec.lyricColumnMaxWidth = 640;
            spec.lyricsPanelMinHeight = 340;
            spec.stackedBreakpoint = 1180;
            break;
        case 0:
        default:
            break;
    }
    return spec;
}

} // namespace

PlayWidget::PlayWidget(QWidget* parent, QWidget* mainWidget)
    : QWidget(parent),
      m_playbackViewModel(new PlaybackViewModel(this)), // ViewModel是UI层的唆一接口
      currentSession(nullptr), lastSeekPosition(0)

{
    // qDebug() << __FUNCTION__ << QThread::currentThread()->currentThreadId;

    qDebug() << "[MVVM-UI] PlayWidget: Initializing with ViewModel-only architecture";

    // 如果没有传入mainWidget，则使用parent
    QWidget* mainParent = mainWidget ? mainWidget : parent;

    // 创建背景图片标签（仅用于存储 pixmap，不直接显示
    backgroundLabel = new QLabel(this);
    backgroundLabel->hide(); // 永远隐藏，只用于存储数据

    // 创建组合的进度条和控制栏（所有功能都在一QML 中）
    process_slider = new ProcessSliderQml(this);
    process_slider->setMinimumHeight(72);
    process_slider->setMaxSeconds(0);
    process_slider->setVolume(m_playbackViewModel->volume());

    // controlBar 现在指向 process_slider（它包含了所有控制功能）
    controlBar = process_slider;

    qDebug() << "ProcessSliderQml created at position:" << process_slider->pos()
             << "size:" << process_slider->size() << "visible:" << process_slider->isVisible();

    desk = new DeskLrcQml(this);
    desk->raise();
    desk->hide();

    // 创建播放历史列表，父窗口为mainWidget
    playlistHistory = new PlaylistHistoryQml(mainParent);
    playlistHistory->hide();

    music = new QPushButton(this);
    music->setFixedSize(30, 30);
    music->setStyleSheet("QPushButton {"
                         "    border-image: url(:/new/prefix1/icon/up.png);"
                         "}");
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
    rotatingCircleShadow = new QGraphicsDropShadowEffect(rotatingCircleHost);
    rotatingCircleShadow->setBlurRadius(42);
    rotatingCircleShadow->setOffset(0, 18);
    rotatingCircleShadow->setColor(QColor(16, 24, 38, 72));
    rotatingCircleHost->setGraphicsEffect(rotatingCircleShadow);
    rotatingCircle = new RotatingCircleImage(rotatingCircleHost);
    rotatingCircle->setAttribute(Qt::WA_TranslucentBackground, true);
    rotatingCircle->setAutoFillBackground(false);
    rotatingCircle->setStyleSheet("background: transparent;");
    rotatingCircle->resize(300, 300);

    squareCoverHost = new QWidget(this);
    squareCoverHost->setAttribute(Qt::WA_TranslucentBackground, true);
    squareCoverHost->setAutoFillBackground(false);
    squareCoverHost->setStyleSheet("background: rgba(255,255,255,0.18);"
                                   "border: 1px solid rgba(255,255,255,0.28);"
                                   "border-radius: 22px;");
    squareCoverShadow = new QGraphicsDropShadowEffect(squareCoverHost);
    squareCoverShadow->setBlurRadius(54);
    squareCoverShadow->setOffset(0, 20);
    squareCoverShadow->setColor(QColor(12, 18, 28, 92));
    squareCoverHost->setGraphicsEffect(squareCoverShadow);
    squareCoverHost->hide();

    squareCoverLabel = new QLabel(squareCoverHost);
    squareCoverLabel->setAlignment(Qt::AlignCenter);
    squareCoverLabel->setScaledContents(true);
    squareCoverLabel->setStyleSheet("background: transparent; border: 0;");

    // ========== MVVM架构：连接ViewModel信号到UI ==========
    setupPlaybackViewModelConnections();

    nameLabel = new QLabel(this);
    nameLabel->setAttribute(Qt::WA_TranslucentBackground, true);
    nameLabel->setStyleSheet("QLabel { color: white; font-size: 28px; background: transparent; }");
    nameLabel->setWordWrap(true);
    nameLabel->setMinimumHeight(30);
    nameLabel->setAlignment(Qt::AlignCenter);

    artistInfoLabel = new QLabel(this);
    artistInfoLabel->setAttribute(Qt::WA_TranslucentBackground, true);
    artistInfoLabel->setStyleSheet(
        "QLabel { color: rgba(255,255,255,0.82); font-size: 14px; background: transparent; }");
    artistInfoLabel->setAlignment(Qt::AlignCenter);

    sceneLabel = new QLabel(this);
    sceneLabel->setAttribute(Qt::WA_TranslucentBackground, true);
    sceneLabel->setStyleSheet("QLabel { color: rgba(255,255,255,0.70); font-size: 15px; "
                              "font-weight: 600; background: transparent; }");
    sceneLabel->setAlignment(Qt::AlignCenter);
    sceneLabel->setText(stageSubtitleText());

    // ========== 新架构状态更新已在前面通过 audioService 信号处理 ==========
    setupCoreConnections();

    setupControlAndPlaylistConnections();

    m_playerPageStyle = SettingsManager::instance().playerPageStyle();
    connect(&SettingsManager::instance(), &SettingsManager::playerPageStyleChanged, this,
            &PlayWidget::handlePlayerPageStyleChanged);
    applyPlayerPageStyle();
    refreshStageTexts();

    updateAdaptiveLayout();
}

void PlayWidget::handleMusicButtonClicked() {
    // 切换状态而不是总是收起。
    emit signalBigClicked(!isUp);
}

void PlayWidget::handleBufferingStateChanged(bool active) {
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
    refreshStageTexts();
}

void PlayWidget::handleProcessChangeRequested(qint64 milliseconds, bool back_flag) {
    Q_UNUSED(back_flag);
    qDebug() << "Seeking to position:" << milliseconds << "ms";
    if (!m_playbackViewModel) {
        qDebug() << "No active ViewModel to seek";
        return;
    }
    m_playbackViewModel->seekTo(milliseconds);
    lastSeekPosition = milliseconds;
}

void PlayWidget::handleDeferredSeekAfterPlay() {
    if (!m_playbackViewModel || pendingSeekPositionMs < 0) {
        return;
    }
    m_playbackViewModel->seekTo(pendingSeekPositionMs);
    pendingSeekPositionMs = -1;
}

void PlayWidget::handleSliderMoveRequested(int seconds) {
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

void PlayWidget::handleCoverExpandRequested() {
    qDebug() << "[PlayWidget] signalUpClick received, isUp =" << isUp;
    // 底部封面区点击仅负责“进入歌词页”，避免在已展开态被误触后直接收起。
    if (!isUp) {
        emit signalBigClicked(true);
    }
}

void PlayWidget::handleLyricPositionChanged() {
    refreshCurrentLyricHighlight(false);
}

int PlayWidget::resolveTargetLyricLine(qint64 positionMs) const {
    if (lyrics.empty()) {
        return -1;
    }

    // 歌词提前 1000ms 显示，让听感与视觉更同步。
    const qint64 safePositionMs = qMax<qint64>(0, positionMs);
    const int timeInMs = static_cast<int>(safePositionMs) + 1000;
    const auto firstLyric = lyrics.begin();

    // 在第一句歌词到来前，固定高亮第一句歌词。
    if (timeInMs <= firstLyric->first) {
        return 5;
    }

    const auto upper = lyrics.upper_bound(timeInMs);
    if (upper == lyrics.begin()) {
        return 5;
    }

    const auto current = (upper == lyrics.end()) ? std::prev(lyrics.end()) : std::prev(upper);
    return static_cast<int>(std::distance(lyrics.begin(), current)) + 5;
}

void PlayWidget::refreshCurrentLyricHighlight(bool forceRecenter) {
    if (!lyricDisplay) {
        return;
    }

    const qint64 positionMs = m_playbackViewModel ? m_playbackViewModel->position() : 0;
    const int targetLine = resolveTargetLyricLine(positionMs);
    if (targetLine < 0) {
        return;
    }

    const bool changed = (targetLine != lyricDisplay->currentLine);
    if (changed) {
        lyricDisplay->highlightLine(targetLine);
        lyricDisplay->currentLine = targetLine;
    }

    if (changed || forceRecenter) {
        lyricDisplay->scrollToLine(targetLine);
        if (forceRecenter) {
            qDebug() << "Lyric highlight refreshed on panel open. line:" << targetLine
                     << "position:" << positionMs << "ms";
        }
        update();
    }
}

void PlayWidget::handleSimilarPlayRequested(const QVariantMap& item) {
    emit signalSimilarSongSelected(item);
}

void PlayWidget::handlePlayerPageStyleRequested(int styleId) {
    SettingsManager::instance().setPlayerPageStyle(styleId);
}

QString PlayWidget::displayArtistText() const {
    if (!currentSongArtist.trimmed().isEmpty()) {
        return currentSongArtist.trimmed();
    }
    if (!networkSongArtist.trimmed().isEmpty()) {
        return networkSongArtist.trimmed();
    }
    return QStringLiteral(u"未知艺术家");
}

void PlayWidget::refreshStageTexts() {
    if (!nameLabel || !artistInfoLabel || !sceneLabel) {
        return;
    }

    const QString title = !currentSongTitle.trimmed().isEmpty()
                              ? currentSongTitle.trimmed()
                              : displayTitleFromFileName(fileName);

    if (!title.isEmpty()) {
        nameLabel->setText(title);
        process_slider->setSongName(title);
        if (desk) {
            desk->setSongName(title);
        }
    }

    artistInfoLabel->setText(displayArtistText());
    sceneLabel->setText(stageSubtitleText());
    sceneLabel->setVisible(isUp && !sceneLabel->text().trimmed().isEmpty());

    if (lyricDisplay) {
        lyricDisplay->setSongInfo(title, displayArtistText());
    }
}

void PlayWidget::queuePlayButtonStateUpdate(bool playing, const QString& path) {
    m_pendingPlayButtonStateValid = true;
    m_pendingPlayButtonPlaying = playing;
    m_pendingPlayButtonPath = path;
    QTimer::singleShot(0, this, &PlayWidget::handleDeferredPlayButtonStateUpdate);
}

void PlayWidget::handleDeferredPlayButtonStateUpdate() {
    if (!m_pendingPlayButtonStateValid) {
        return;
    }

    const bool playing = m_pendingPlayButtonPlaying;
    const QString path = m_pendingPlayButtonPath;
    m_pendingPlayButtonStateValid = false;
    m_pendingPlayButtonPath.clear();
    emit signalPlayButtonClick(playing, path);
}

void PlayWidget::shutdownQuickWidget(QQuickWidget* widget) {
    if (!widget) {
        return;
    }
    widget->setUpdatesEnabled(false);
    if (QQuickWindow* quickWindow = widget->quickWindow()) {
        quickWindow->setPersistentOpenGLContext(false);
        quickWindow->setPersistentSceneGraph(false);
    }
    widget->hide();
}

void PlayWidget::onDeskToggled(bool checked) {
    if (checked) {
        desk->show();
    } else {
        desk->hide();
    }
}
void PlayWidget::setIsUp(bool flag) {
    isUp = flag;

    qDebug() << "PlayWidget::setIsUp called with flag:" << flag;
    qDebug() << "Call stack trace - isUp:" << flag;

    // 展开时如果还没有封面背景，使用深色兜底，避免出现白底。
    if (flag && backgroundLabel &&
        (!backgroundLabel->pixmap() || backgroundLabel->pixmap()->isNull())) {
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

    // 展开播放页时始终显示歌词区域，不再把前四种风格隐藏掉。
    if (lyricDisplay) {
        if (flag) {
            qDebug() << "Showing lyric display";
            lyricDisplay->show();
            lyricDisplay->setIsUp(true);
            // 每次展开歌词面板都重新定位一次高亮行与滚动位置。
            refreshCurrentLyricHighlight(true);
        } else {
            qDebug() << "Hiding lyric display";
            lyricDisplay->hide(); // 收起时隐藏歌
            lyricDisplay->setIsUp(false);
        }
    }

    if (nameLabel) {
        nameLabel->setVisible(false);
    }
    if (artistInfoLabel) {
        artistInfoLabel->setVisible(false);
    }
    if (sceneLabel) {
        sceneLabel->setVisible(false);
    }

    // 触发重绘以更新背
    update();

    updateAdaptiveLayout();

    emit signalIsUpChanged(isUp);
}

void PlayWidget::handlePlayerPageStyleChanged() {
    m_playerPageStyle = SettingsManager::instance().playerPageStyle();
    applyPlayerPageStyle();
}

void PlayWidget::invalidateStageBackgroundCache() {
    m_stageBackgroundCacheDirty = true;
}

void PlayWidget::rebuildStageBackgroundCache() {
    const QSize targetSize = size();
    if (targetSize.isEmpty()) {
        m_stageBackgroundCache = QPixmap();
        m_stageBackgroundCacheDirty = false;
        return;
    }

    QPixmap cache(targetSize);
    cache.fill(Qt::transparent);

    QPainter painter(&cache);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (isUp) {
        if (m_playerPageStyle == 4 && !m_originalBackgroundPixmap.isNull()) {
            painter.drawPixmap(QRect(QPoint(0, 0), targetSize), m_originalBackgroundPixmap);
            painter.fillRect(QRect(QPoint(0, 0), targetSize), QColor(8, 10, 14, 86));
        } else {
            QLinearGradient gradient(0, 0, targetSize.width(), targetSize.height());
            switch (m_playerPageStyle) {
                case 0:
                    gradient.setColorAt(0.0, QColor("#D7F6FA"));
                    gradient.setColorAt(0.52, QColor("#D9F7FA"));
                    gradient.setColorAt(1.0, QColor("#D2F1F6"));
                    break;
                case 1:
                    gradient.setColorAt(0.0, QColor("#1A353C"));
                    gradient.setColorAt(0.55, QColor("#19343B"));
                    gradient.setColorAt(1.0, QColor("#173038"));
                    break;
                case 2:
                    gradient.setColorAt(0.0, QColor("#ECECF0"));
                    gradient.setColorAt(0.50, QColor("#ECECF0"));
                    gradient.setColorAt(1.0, QColor("#EFEFF2"));
                    break;
                case 3:
                    gradient.setColorAt(0.0, QColor("#19343B"));
                    gradient.setColorAt(0.5, QColor("#173038"));
                    gradient.setColorAt(1.0, QColor("#152D34"));
                    break;
                default:
                    gradient.setColorAt(0.0, QColor("#F5F5F7"));
                    gradient.setColorAt(0.5, QColor("#FAFAFA"));
                    gradient.setColorAt(1.0, QColor("#F0F0F2"));
                    break;
            }
            painter.fillRect(QRect(QPoint(0, 0), targetSize), gradient);
            if ((m_playerPageStyle == 0 || m_playerPageStyle == 2) && backgroundLabel->pixmap() &&
                !backgroundLabel->pixmap()->isNull()) {
                painter.setOpacity(m_playerPageStyle == 2 ? 0.16 : 0.10);
                painter.drawPixmap(QRect(QPoint(0, 0), targetSize), *backgroundLabel->pixmap());
                painter.setOpacity(1.0);
            }
        }

        if (m_playerPageStyle == 0 && rotatingCircleHost && rotatingCircleHost->isVisible()) {
            const QRect turntableRect = rotatingCircleHost->geometry().adjusted(-28, -28, 28, 28);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(128, 154, 164, 24));
            painter.drawRoundedRect(turntableRect.translated(0, 22), 34, 34);

            painter.setBrush(QColor(248, 251, 252, 248));
            painter.drawRoundedRect(turntableRect, 34, 34);

            painter.setPen(QPen(QColor(255, 255, 255, 120), 2));
            painter.drawRoundedRect(turntableRect.adjusted(8, 8, -8, -8), 28, 28);

            const QPoint armPivot(turntableRect.right() - 40, turntableRect.top() + 54);
            const QPoint armMid(turntableRect.right() - 42, turntableRect.bottom() - 120);
            const QPoint armHead(turntableRect.right() - 82, turntableRect.bottom() - 54);

            painter.setPen(
                QPen(QColor(92, 98, 102, 210), 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.drawLine(armPivot, armMid);
            painter.setPen(
                QPen(QColor(188, 190, 192, 224), 7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.drawLine(armMid, armHead);

            painter.setBrush(QColor(240, 241, 243, 245));
            painter.setPen(QPen(QColor(180, 186, 190, 160), 1));
            painter.drawEllipse(armPivot, 18, 18);
            painter.drawEllipse(armHead, 10, 10);

            const QRect badgeRect(turntableRect.right() - 48, turntableRect.bottom() - 48, 28, 28);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(255, 255, 255, 238));
            painter.drawEllipse(badgeRect);
            painter.setPen(QPen(QColor("#24D05A"), 3));
            painter.drawArc(badgeRect.adjusted(6, 6, -6, -6), 40 * 16, 250 * 16);
        } else if (m_playerPageStyle == 2 && rotatingCircleHost &&
                   rotatingCircleHost->isVisible()) {
            const QRect glowRect = rotatingCircleHost->geometry().adjusted(-90, -90, 120, 120);
            QRadialGradient glow(glowRect.center(), glowRect.width() / 2.0);
            glow.setColorAt(0.0, QColor(255, 222, 123, 0));
            glow.setColorAt(0.56, QColor(255, 215, 87, 36));
            glow.setColorAt(0.82, QColor(247, 202, 61, 96));
            glow.setColorAt(1.0, QColor(247, 202, 61, 0));
            painter.setPen(Qt::NoPen);
            painter.setBrush(glow);
            painter.drawEllipse(glowRect);
        }
    } else {
        QLinearGradient gradient(0, 0, targetSize.width(), targetSize.height());
        gradient.setColorAt(0, QColor("#F5F5F7"));
        gradient.setColorAt(0.5, QColor("#FAFAFA"));
        gradient.setColorAt(1, QColor("#F0F0F2"));
        painter.fillRect(QRect(QPoint(0, 0), targetSize), gradient);
    }

    painter.end();
    m_stageBackgroundCache = cache;
    m_stageBackgroundCacheDirty = false;
}

void PlayWidget::applyPlayerPageStyle() {
    if (process_slider) {
        process_slider->setPlayerPageStyle(m_playerPageStyle);
    }
    if (lyricDisplay) {
        lyricDisplay->setPlayerPageStyle(m_playerPageStyle);
    }

    const bool lightThemeStage = (m_playerPageStyle == 0 || m_playerPageStyle == 2);
    const QString titleColor =
        lightThemeStage ? QStringLiteral("#11181F") : QStringLiteral("#F5F8FD");
    const QString artistColor = lightThemeStage ? QStringLiteral("rgba(66,78,92,0.76)")
                                                : QStringLiteral("rgba(240,246,252,0.72)");
    const QString sceneColor = lightThemeStage ? QStringLiteral("rgba(75,86,102,0.78)")
                                               : QStringLiteral("rgba(230,238,244,0.58)");
    nameLabel->setStyleSheet(
        QStringLiteral(
            "QLabel { color: %1; font-size: 28px; font-weight: 600; background: transparent; }")
            .arg(titleColor));
    artistInfoLabel->setStyleSheet(
        QStringLiteral(
            "QLabel { color: %1; font-size: 14px; font-weight: 500; background: transparent; }")
            .arg(artistColor));
    sceneLabel->setStyleSheet(
        QStringLiteral(
            "QLabel { color: %1; font-size: 15px; font-weight: 600; background: transparent; }")
            .arg(sceneColor));

    const bool hideCover = (m_playerPageStyle == 3);
    const bool useSquareCover = !hideCover && (m_playerPageStyle == 1 || m_playerPageStyle == 4);
    if (rotatingCircleHost) {
        rotatingCircleHost->setVisible(!hideCover && !useSquareCover);
    }
    if (rotatingCircle) {
        RotatingCircleImage::DiscVisualStyle discStyle =
            RotatingCircleImage::DiscVisualStyle::DarkVinyl;
        qreal coverScale = 0.70;
        if (m_playerPageStyle == 0) {
            discStyle = RotatingCircleImage::DiscVisualStyle::SilverVinyl;
            coverScale = 0.56;
        } else if (m_playerPageStyle == 2) {
            discStyle = RotatingCircleImage::DiscVisualStyle::GoldVinyl;
            coverScale = 0.52;
        }
        rotatingCircle->setDiscVisualStyle(discStyle);
        rotatingCircle->setCoverScale(coverScale);
    }
    if (squareCoverHost) {
        squareCoverHost->setVisible(useSquareCover);
        if (m_playerPageStyle == 1) {
            squareCoverHost->setStyleSheet("background: transparent;"
                                           "border: 0;"
                                           "border-radius: 18px;");
            if (squareCoverShadow) {
                squareCoverShadow->setEnabled(false);
            }
        } else if (m_playerPageStyle == 4) {
            squareCoverHost->setStyleSheet("background: rgba(12,16,22,0.28);"
                                           "border: 1px solid rgba(255,255,255,0.22);"
                                           "border-radius: 28px;");
            if (squareCoverShadow) {
                squareCoverShadow->setEnabled(false);
            }
        } else {
            squareCoverHost->setStyleSheet("background: rgba(255,255,255,0.28);"
                                           "border: 1px solid rgba(255,255,255,0.38);"
                                           "border-radius: 24px;");
            if (squareCoverShadow) {
                squareCoverShadow->setEnabled(false);
            }
        }
    }

    if (rotatingCircleShadow) {
        rotatingCircleShadow->setEnabled(false);
    }

    if (lyricDisplay) {
        if (lyricDisplay->rootObject()) {
            lyricDisplay->rootObject()->setProperty("showSongInfo", true);
        }
        lyricDisplay->setVisible(isUp);
    }
    nameLabel->setVisible(false);
    artistInfoLabel->setVisible(false);
    sceneLabel->setVisible(false);

    refreshStageTexts();
    updateAdaptiveLayout();
    invalidateStageBackgroundCache();
    update();
}

void PlayWidget::updateCoverPresentation(const QString& imagePath) {
    if (process_slider) {
        process_slider->setPicPath(imagePath);
    }
    if (rotatingCircle) {
        rotatingCircle->setImage(imagePath);
    }

    if (!squareCoverLabel) {
        return;
    }

    const QString trimmedPath = imagePath.trimmed();
    QString localPath = trimmedPath;
    if (trimmedPath.startsWith("file:///")) {
        localPath = QUrl(trimmedPath).toLocalFile();
    } else if (trimmedPath.startsWith("qrc:")) {
        localPath = trimmedPath.mid(3);
    }

    QPixmap cover(localPath);
    if (cover.isNull()) {
        cover = defaultCoverPixmapForSize(squareCoverLabel->size());
    }
    if (!cover.isNull()) {
        squareCoverLabel->setPixmap(cover);
    }
}

QPixmap PlayWidget::defaultCoverPixmapForSize(const QSize& requestedSize) const {
    QSize safeSize = requestedSize;
    if (safeSize.width() <= 0 || safeSize.height() <= 0) {
        safeSize = QSize(220, 220);
    }

    const qreal dpr = devicePixelRatioF();
    const QSize deviceSize(qMax(1, qRound(safeSize.width() * dpr)),
                           qMax(1, qRound(safeSize.height() * dpr)));
    QPixmap cover = QIcon(QStringLiteral(":/qml/assets/ai/icons/default-music-cover.svg"))
                        .pixmap(deviceSize);
    if (!cover.isNull()) {
        cover.setDevicePixelRatio(dpr);
    }
    return cover;
}

int PlayWidget::collapsedPlaybackHeight() const {
    return process_slider ? process_slider->height() : 90;
}

QVariantMap PlayWidget::desktopLyricSnapshot() const {
    if (!desk) {
        return {{QStringLiteral("available"), false}};
    }

    return {{QStringLiteral("available"), true},
            {QStringLiteral("visible"), desk->isVisible()},
            {QStringLiteral("songName"), desk->songName()},
            {QStringLiteral("lyricText"), desk->lyricText()},
            {QStringLiteral("color"), desk->lyricStyleColor().name(QColor::HexRgb)},
            {QStringLiteral("fontSize"), desk->lyricStyleFontSize()},
            {QStringLiteral("fontFamily"), desk->lyricStyleFontFamily()}};
}

bool PlayWidget::setDesktopLyricVisible(bool visible) {
    if (!desk) {
        return false;
    }

    if (visible) {
        desk->show();
        desk->raise();
        desk->activateWindow();
    } else {
        desk->hide();
    }
    return desk->isVisible() == visible;
}

bool PlayWidget::setDesktopLyricStyleFromMap(const QVariantMap& style) {
    if (!desk) {
        return false;
    }

    QColor color(style.value(QStringLiteral("color")).toString().trimmed());
    if (!color.isValid()) {
        color = QColor(QStringLiteral("#FFFFFF"));
    }

    const int fontSize = qBound(12, style.value(QStringLiteral("fontSize"), 18).toInt(), 48);
    const QString family = style.value(QStringLiteral("fontFamily")).toString().trimmed();
    QFont font(family.isEmpty() ? QStringLiteral("Microsoft YaHei") : family);
    desk->setLyricStyle(color, fontSize, font);
    return true;
}

void PlayWidget::updateAdaptiveLayout() {
    const int wWidth = qMax(1, width());
    const int wHeight = qMax(1, height());
    const StageLayoutSpec layoutSpec = stageLayoutSpecForStyle(m_playerPageStyle);

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
    const int stageTop = qMin(contentBottom - 80, contentTop + layoutSpec.stageTopMargin);
    const QRect stageRect(layoutSpec.stageHorizontalMargin, stageTop,
                          qMax(120, wWidth - layoutSpec.stageHorizontalMargin * 2),
                          qMax(120, contentBottom - stageTop));

    const bool discVisible = rotatingCircleHost && rotatingCircleHost->isVisible();
    const bool squareVisible = squareCoverHost && squareCoverHost->isVisible();
    const bool hasCover = discVisible || squareVisible;
    const bool stackedLayout = (wWidth < layoutSpec.stackedBreakpoint);

    QRect coverRect;
    QRect lyricRect = stageRect;

    if (layoutSpec.lyricsFocus || !hasCover) {
        const int lyricWidth = qMin(layoutSpec.lyricColumnMaxWidth, stageRect.width());
        const int lyricHeight =
            qMin(stageRect.height(), qMax(layoutSpec.lyricsPanelMinHeight, stageRect.height() - 8));
        lyricRect = QRect(stageRect.x() + qMax(0, (stageRect.width() - lyricWidth) / 2),
                          stageRect.y() + qMax(0, (stageRect.height() - lyricHeight) / 2),
                          lyricWidth, lyricHeight);
    } else if (!stackedLayout) {
        int coverColumnWidth =
            static_cast<int>(stageRect.width() * layoutSpec.coverColumnWidthRatio);
        coverColumnWidth = qBound(
            layoutSpec.coverMinSize + 36, coverColumnWidth,
            qMax(layoutSpec.coverMinSize + 36, stageRect.width() - layoutSpec.columnGap - 360));
        int lyricWidth = qMax(360, stageRect.width() - coverColumnWidth - layoutSpec.columnGap);
        lyricWidth = qMin(layoutSpec.lyricColumnMaxWidth, lyricWidth);

        const QRect coverColumn(stageRect.x(), stageRect.y(), coverColumnWidth, stageRect.height());
        const int lyricX = coverColumn.right() + 1 + layoutSpec.columnGap;
        lyricRect = QRect(lyricX, stageRect.y(), qMax(320, stageRect.right() - lyricX + 1),
                          qMax(layoutSpec.lyricsPanelMinHeight, stageRect.height()));
        if (lyricRect.width() > lyricWidth) {
            lyricRect.moveLeft(stageRect.right() - lyricWidth + 1);
            lyricRect.setWidth(lyricWidth);
        }

        const int coverSize = qBound(
            layoutSpec.coverMinSize,
            qMin(layoutSpec.coverMaxSize, qMin(coverColumn.width(), stageRect.height() - 20)),
            layoutSpec.coverMaxSize);
        coverRect = QRect(coverColumn.x() + qMax(0, (coverColumn.width() - coverSize) / 2),
                          coverColumn.y() + qMax(0, (coverColumn.height() - coverSize) / 2),
                          coverSize, coverSize);
    } else {
        const int coverSize =
            qBound(layoutSpec.coverMinSize,
                   qMin(layoutSpec.coverMaxSize,
                        qMin(stageRect.width() - 40, static_cast<int>(stageRect.height() * 0.42))),
                   layoutSpec.coverMaxSize);
        coverRect = QRect(stageRect.x() + qMax(0, (stageRect.width() - coverSize) / 2),
                          stageRect.y(), coverSize, coverSize);

        const int lyricWidth = qMin(layoutSpec.lyricColumnMaxWidth, stageRect.width());
        const int lyricTop = coverRect.bottom() + 1 + layoutSpec.stackedGap;
        const int lyricHeight = qMax(layoutSpec.lyricsPanelMinHeight, contentBottom - lyricTop);
        lyricRect = QRect(stageRect.x() + qMax(0, (stageRect.width() - lyricWidth) / 2), lyricTop,
                          lyricWidth, qMin(lyricHeight, contentBottom - lyricTop));
    }

    if (rotatingCircleHost) {
        rotatingCircleHost->setGeometry(discVisible ? coverRect : QRect());
    }
    if (rotatingCircle) {
        rotatingCircle->setGeometry(0, 0, rotatingCircleHost ? rotatingCircleHost->width() : 0,
                                    rotatingCircleHost ? rotatingCircleHost->height() : 0);
    }

    if (squareCoverHost) {
        squareCoverHost->setGeometry(squareVisible ? coverRect : QRect());
    }
    if (squareCoverLabel && squareCoverHost) {
        const int inset = qMax(14, squareCoverHost->width() / 12);
        squareCoverLabel->setGeometry(inset, inset, qMax(1, squareCoverHost->width() - inset * 2),
                                      qMax(1, squareCoverHost->height() - inset * 2));
        if (squareVisible) {
            QPixmap cover;
            const QString currentCover =
                m_playbackViewModel ? m_playbackViewModel->currentAlbumArt().trimmed() : QString();
            QString localPath = currentCover;
            if (currentCover.startsWith("file:///")) {
                localPath = QUrl(currentCover).toLocalFile();
            } else if (currentCover.startsWith("qrc:")) {
                localPath = currentCover.mid(3);
            }
            if (!localPath.isEmpty()) {
                cover.load(localPath);
            }
            if (cover.isNull()) {
                cover = defaultCoverPixmapForSize(squareCoverLabel->size());
            }
            if (!cover.isNull()) {
                squareCoverLabel->setPixmap(cover);
            }
        }
    }

    if (nameLabel) {
        nameLabel->hide();
        nameLabel->setGeometry(0, 0, 0, 0);
    }
    if (artistInfoLabel) {
        artistInfoLabel->hide();
        artistInfoLabel->setGeometry(0, 0, 0, 0);
    }
    if (sceneLabel) {
        sceneLabel->hide();
        sceneLabel->setGeometry(0, 0, 0, 0);
    }

    if (lyricDisplay) {
        lyricDisplay->setGeometry(lyricRect);

        const double playbackAreaCenterY = (wHeight - controlHeight) / 2.0;
        const double localCenterY = playbackAreaCenterY - lyricRect.y();
        const double centerOffsetY = localCenterY - lyricRect.height() / 2.0;
        lyricDisplay->setCenterYOffset(centerOffsetY);
    }

    if (isUp) {
        clearMask();
    } else {
        setMask(QRegion(0, wHeight - controlHeight, wWidth, controlHeight));
    }

    invalidateStageBackgroundCache();
}

void PlayWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateAdaptiveLayout();
    invalidateStageBackgroundCache();
}

void PlayWidget::onWorkStop() {
    qDebug() << __FUNCTION__ << "暂停";
    emit signalPlayState(ProcessSliderQml::Pause);
    if (filePath.size()) {
        emit signalStopRotate(false);
        qDebug() << "[PLAY_STATE] onWorkStop 发出 signalPlayButtonClick(false," << filePath << ")";

        // 延迟 UI 列表更新，避免阻塞主播放链路。
        queuePlayButtonStateUpdate(false, filePath);

        const QString title = !currentSongTitle.trimmed().isEmpty()
                                  ? currentSongTitle.trimmed()
                                  : displayTitleFromFileName(fileName);
        process_slider->setSongName(title);
        refreshStageTexts();
    }
}
void PlayWidget::onWorkPlay() {
    emit signalPlayState(ProcessSliderQml::Play);
    emit signalStopRotate(true);
    qDebug() << "[PLAY_STATE] onWorkPlay 发出 signalPlayButtonClick(true," << filePath << ")";

    // 延迟更新 UI 列表，确保音频完全启动后再更新状态。
    queuePlayButtonStateUpdate(true, filePath);

    const QString title = !currentSongTitle.trimmed().isEmpty()
                              ? currentSongTitle.trimmed()
                              : displayTitleFromFileName(fileName);
    process_slider->setSongName(title);
    refreshStageTexts();
}
void PlayWidget::onLrcSendLrc(const std::map<int, std::string> lyrics) {
    this->lyricDisplay->currentLine = 5; // 初始行设，对应第一行歌
    this->lyrics.clear();

    {
        std::lock_guard<std::mutex> lock(mtx);
        this->lyrics = lyrics;
    }

    // 设置歌曲信息到歌词显示界
    if (!fileName.isEmpty() || !currentSongTitle.trimmed().isEmpty()) {
        const QString songName = !currentSongTitle.trimmed().isEmpty()
                                     ? currentSongTitle.trimmed()
                                     : displayTitleFromFileName(fileName);
        lyricDisplay->setSongInfo(songName, displayArtistText());
    }

    // 使用带时间的 std::map 格式设置歌词
    lyricDisplay->setLyrics(lyrics);

    // 歌词数据刚加载后，主动按当前播放时间刷新一次高亮行。
    // 这样即使 positionChanged 还没到来，展开歌词页也能拿到正确定位。
    QTimer::singleShot(0, this, [this]() {
        refreshCurrentLyricHighlight(true);
    });

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
void PlayWidget::onPlayClick() {
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
                QUrl url =
                    m_playbackViewModel->currentFilePath().startsWith("http", Qt::CaseInsensitive)
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

void PlayWidget::rePlay(QString path) {
    if (path.size() == 0)
        return;
    emit signalFilepath(path);
}

void PlayWidget::initLyricDisplay() {
    qDebug() << "Initializing LyricDisplay...";
    lyricDisplay = new LyricDisplayQml(this);
    lyricDisplay->setClearColor(QColor(Qt::transparent));
    lyricDisplay->setAttribute(Qt::WA_AlwaysStackOnTop, true);
    lyricDisplay->setStyleSheet("background: transparent; border: 0;");
    lyricDisplay->setAutoFillBackground(false);
    lyricDisplay->setContentsMargins(0, 0, 0, 0);
    lyricDisplay->setMinimumSize(360, 220);
    lyricDisplay->raise();
    if (lyricDisplay->rootObject()) {
        lyricDisplay->rootObject()->setProperty("showSongInfo", true);
    }
    lyricDisplay->hide(); // 初始时隐藏歌词，只有展开时才显示
    qDebug() << "LyricDisplay initialized, size:" << lyricDisplay->size()
             << "position:" << lyricDisplay->pos();
}

void PlayWidget::beginTakeLrc(QString str) {
    this->lyricDisplay->clearLyrics();
    lrc->begin_take_lrc(str);
}
void PlayWidget::openfile() {

    // 创建一个临时的 QWidget 实例，用于显示文件对话框
    QWidget dummyWidget;

    // 打开文件对话
    QString filePath_ = QFileDialog::getOpenFileName(
        &dummyWidget,                                            // 父窗口（可以nullptr
        QStringLiteral(u"\u9009\u62e9\u97f3\u9891\u6587\u4ef6"), // 对话框标题
        QDir::homePath(),                                        // 起始目录（可以是任意路径
        QStringLiteral(u"\u97f3\u9891\u6587\u4ef6 (*.mp3 *.wav *.flac *.ogg "
                       u"*mp4);;\u6240\u6709\u6587\u4ef6 (*)") // 文件过滤
    );

    // 打印选中的文件路
    if (!filePath_.isEmpty()) {
        QFileInfo fileInfo(filePath_);
        QString filename = fileInfo.fileName();
        emit signalAddSong(filename, filePath_);
    }
}

void PlayWidget::playClick(QString songPath) {
    const QString normalizedSongPath = normalizeMediaPathForPlayback(songPath);
    const QString normalizedCurrentPath = normalizeMediaPathForPlayback(this->filePath);

    qDebug() << "[MVVM-UI] playClick compare - input:" << songPath
             << "normalizedInput:" << normalizedSongPath << "current:" << this->filePath
             << "normalizedCurrent:" << normalizedCurrentPath;

    if (normalizedSongPath != normalizedCurrentPath) {
        this->filePath = normalizedSongPath;

        fileName = decodedFileNameFromPath(normalizedSongPath);
        if (!checkAndWarnIfPathNotExists(normalizedSongPath))
            return;

        // ========== MVVM架构：通过ViewModel播放 ==========
        qDebug() << "[MVVM-UI] playClick: Playing via ViewModel:" << normalizedSongPath;

        // 使用ViewModel播放
        QUrl url;
        if (normalizedSongPath.startsWith("http", Qt::CaseInsensitive)) {
            url = QUrl(normalizedSongPath); // 网络URL
        } else {
            url = QUrl::fromLocalFile(normalizedSongPath); // 本地文件
        }

        m_playbackViewModel->play(url);

        // ViewModel会自动：
        // 1. 调用AudioService播放
        // 2. 更新内部状态（isPlaying, position, duration等）
        // 3. 发出shouldLoadLyrics信号触发歌词加载
        // 4. 发出shouldStartRotation信号触发旋转动画

        // 这里只需要处理UI层特有的逻辑
        emit signalBeginToPlay(normalizedSongPath);
    } else {
        // 相同路径，切换播暂停
        onPlayClick();
    }
}
void PlayWidget::removeClick(QString songName) {
    if (songName == this->filePath) {
        this->fileName.clear();
        this->filePath.clear();

        emit signalRemoveClick();
    }
}
void PlayWidget::setPianWidgetEnable(bool flag) {
    // PianWidget 现在集成ProcessSliderQml 中，不需要单独控
}
bool PlayWidget::checkAndWarnIfPathNotExists(const QString& path) {

    if (path.startsWith("http", Qt::CaseInsensitive)) {
        qDebug() << "检测到网络路径，跳过存在性检" << path;
        return true;
    }

    QFileInfo fileInfo(path);

    if (fileInfo.exists()) {
        return true;
    } else {
        QMessageBox::warning(
            nullptr, QStringLiteral(u"\u8def\u5f84\u4e0d\u5b58\u5728"),
            QStringLiteral(
                u"\u6587\u4ef6\u4e0d\u5b58\u5728\u6216\u65e0\u6cd5\u8bbf\u95ee\uff1a\n%1")
                .arg(path));
        return false;
    }
}
PlayWidget::~PlayWidget() {
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
    if (b) {
        qDebug() << "PlayWidget: Stopping lyric thread b...";
        b->quit();
        if (!b->wait(2000)) {
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
        localPath = picPath.mid(3); // 去掉"qrc"，保:/..."
        qDebug() << "Converted QRC path:" << localPath;
    }

    // 加载原始图片
    QPixmap originalPixmap(localPath);
    if (originalPixmap.isNull()) {
        qDebug() << "Failed to load album cover for background:" << localPath;
        // 使用默认背景图片
        qDebug() << "Using default background image";
        originalPixmap = QIcon(QStringLiteral(":/qml/assets/ai/icons/default-music-cover.svg"))
                             .pixmap(QSize(256, 256));
        if (originalPixmap.isNull()) {
            qDebug() << "Failed to load default background image, skipping";
            return;
        }
    }

    qDebug() << "Original pixmap size:" << originalPixmap.size();

    const int targetWidth = qMax(640, width());
    const int targetHeight = qMax(360, height());

    // 1. 按当前窗口尺寸缩放并填满
    QPixmap scaledPixmap = originalPixmap.scaled(
        targetWidth, targetHeight, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    // 2. 裁剪到目标尺寸中心区域
    int x = (scaledPixmap.width() - targetWidth) / 2;
    int y = (scaledPixmap.height() - targetHeight) / 2;
    QPixmap croppedPixmap = scaledPixmap.copy(x, y, targetWidth, targetHeight);

    qDebug() << "Cropped pixmap size:" << croppedPixmap.size();

    m_originalBackgroundPixmap = croppedPixmap;

    // 3. 创建强模糊效果的图片 - 多次缩放模拟高斯模糊
    QImage image = croppedPixmap.toImage();

    // 使用分段缩放生成模糊背景，分辨率随窗口尺寸自适应
    const int blurW1 = qMax(96, targetWidth / 10);
    const int blurH1 = qMax(54, targetHeight / 10);
    const int blurW2 = qMax(192, targetWidth / 5);
    const int blurH2 = qMax(108, targetHeight / 5);
    QImage smallImage1 =
        image.scaled(blurW1, blurH1, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QImage smallImage2 =
        smallImage1.scaled(blurW2, blurH2, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QImage blurredImage = smallImage2.scaled(targetWidth, targetHeight, Qt::IgnoreAspectRatio,
                                             Qt::SmoothTransformation);

    // 4. 根据当前样式添加遮罩，控制沉浸感与歌词可读性。
    QPainter maskPainter(&blurredImage);
    maskPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    int overlayAlpha = 140;
    if (m_playerPageStyle == 0) {
        overlayAlpha = 38;
    } else if (m_playerPageStyle == 2) {
        overlayAlpha = 18;
    } else if (m_playerPageStyle == 3) {
        overlayAlpha = 118;
    } else if (m_playerPageStyle == 4) {
        overlayAlpha = 152;
    }
    maskPainter.fillRect(blurredImage.rect(), QColor(0, 0, 0, overlayAlpha));
    maskPainter.end();

    // 5. 转换QPixmap 并设
    QPixmap blurredPixmap = QPixmap::fromImage(blurredImage);
    backgroundLabel->setPixmap(blurredPixmap);

    // 触发重绘，让 paintEvent 使用新的背景
    invalidateStageBackgroundCache();
    update();

    qDebug() << "Background updated successfully, pixmap size:" << blurredPixmap.size();
    qDebug() << "backgroundLabel visible:" << backgroundLabel->isVisible()
             << "geometry:" << backgroundLabel->geometry();
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
    lyricUpdateConnection = connect(m_playbackViewModel, &PlaybackViewModel::positionChanged, this,
                                    &PlayWidget::handleLyricPositionChanged);
}

void PlayWidget::onLyricPreview(int timeMs) {
    // 拖拽预览时显示时间，但不实际跳转
    int seconds = timeMs / 1000;
    int minutes = seconds / 60;
    seconds = seconds % 60;

    QString timeStr =
        QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
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

void PlayWidget::setSimilarRecommendations(const QVariantList& items) {
    if (lyricDisplay) {
        lyricDisplay->setSimilarSongs(items);
    }
}

void PlayWidget::clearSimilarRecommendations() {
    if (lyricDisplay) {
        lyricDisplay->clearSimilarSongs();
    }
}

PlaybackViewModel* PlayWidget::playbackViewModel() const {
    return m_playbackViewModel;
}

bool PlayWidget::getNetFlag() {
    return play_net;
}

void PlayWidget::setPlayNet(bool flag) {
    play_net = flag;
    emit signalNetFlagChanged(flag);
}

void PlayWidget::setNetworkMetadata(const QString& artist, const QString& cover) {
    networkSongArtist = artist;
    networkSongCover = cover;
    refreshStageTexts();
}

void PlayWidget::setNetworkMetadata(const QString& title, const QString& artist,
                                    const QString& cover) {
    currentSongTitle = title;
    currentSongArtist = artist;
    networkSongArtist = artist;
    networkSongCover = cover;
    refreshStageTexts();
}
