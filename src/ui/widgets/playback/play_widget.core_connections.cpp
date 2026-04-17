#include "play_widget.h"

/*
模块名称: PlayWidget 核心连接
功能概述: 收敛构造阶段的核心信号绑定，包含旋转唱片、进度跳转、歌词解析与歌词交互链路。
对外接口: PlayWidget::setupCoreConnections()
维护说明: 仅维护连接关系，避免在构造函数中堆叠过多 connect 语句。
*/

void PlayWidget::setupCoreConnections()
{
    connect(this, &PlayWidget::signalStopRotate,
            rotatingCircle, &TurntableGlWidget::onStopRotate);

    // 歌词更新逻辑（由 ViewModel 转发时间轴和缓冲状态）。
    connect(m_playbackViewModel, &PlaybackViewModel::bufferingStateChanged,
            this, &PlayWidget::handleBufferingStateChanged);

    // 进度跳转信号链路。
    connect(this, &PlayWidget::signalProcessChange,
            this, &PlayWidget::handleProcessChangeRequested);
    connect(process_slider, &ProcessSliderQml::signalSliderMove,
            this, &PlayWidget::handleSliderMoveRequested);
    connect(process_slider, &ProcessSliderQml::signalUpClick,
            this, &PlayWidget::handleCoverExpandRequested);

    // 歌词解析与桌面歌词同步。
    connect(this, &PlayWidget::signalBeginTakeLrc,
            lrc.get(), &LrcAnalyze::begin_take_lrc);
    connect(lrc.get(), &LrcAnalyze::send_lrc,
            this, &PlayWidget::onLrcSendLrc);
    connect(lyricDisplay, &LyricDisplayQml::signalCurrentLrc,
            this, &PlayWidget::signalDeskLrc);
    connect(this, &PlayWidget::signalDeskLrc,
            desk, &DeskLrcQml::setLyricText);

    lyricUpdateConnection = connect(m_playbackViewModel, &PlaybackViewModel::positionChanged,
                                    this, &PlayWidget::handleLyricPositionChanged);

    // 歌词拖动与相似推荐交互。
    connect(lyricDisplay, &LyricDisplayQml::signalLyricSeek,
            this, &PlayWidget::onLyricSeek);
    connect(lyricDisplay, &LyricDisplayQml::signalLyricDragStart,
            this, &PlayWidget::onLyricDragStart);
    connect(lyricDisplay, &LyricDisplayQml::signalLyricDragPreview,
            this, &PlayWidget::onLyricPreview);
    connect(lyricDisplay, &LyricDisplayQml::signalLyricDragEnd,
            this, &PlayWidget::onLyricDragEnd);
    connect(lyricDisplay, &LyricDisplayQml::signalSimilarPlayRequested,
            this, &PlayWidget::handleSimilarPlayRequested);
}
