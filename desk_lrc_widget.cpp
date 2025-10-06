#include "desk_lrc_widget.h"

DeskLrcWidget::DeskLrcWidget(QWidget* parent)
    :QWidget(parent), settingsDialog(nullptr), resizing(false)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true); // 启用鼠标跟踪以支持拉伸
    resize(500, 120);
    setMinimumSize(250, 100);  // 放宽最小尺寸限制
    setMaximumSize(1600, 400); // 放宽最大尺寸限制
    
    controlBar = new MiniControlBar(this);
    controlBar->setFixedHeight(35);
    controlBar->hide();
    
    // 美化控制栏样式
    controlBar->setStyleSheet(
        "MiniControlBar {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                              stop:0 rgba(40, 40, 40, 200), "
        "                              stop:1 rgba(20, 20, 20, 220));"
        "    border-radius: 15px;"
        "    border: 1px solid rgba(255, 255, 255, 0.2);"
        "}"
    );

    lrc = new QLabel("♪ 暂无歌词 ♪", this);
    lrc->setAlignment(Qt::AlignCenter);
    lrc->setWordWrap(true);
    
    // 初始化默认样式 - 更现代化的外观
    QFont font;
    font.setFamily("Microsoft YaHei");
    font.setPointSize(18);
    font.setWeight(QFont::Bold);
    lrc->setFont(font);
    
    // 美化歌词标签样式
    lrc->setStyleSheet(
        "QLabel {"
        "    color: #ffffff;"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                              stop:0 rgba(0, 0, 0, 100), "
        "                              stop:1 rgba(0, 0, 0, 150));"
        "    border-radius: 20px;"
        "    padding: 15px 25px;"
        "    border: 2px solid rgba(255, 255, 255, 0.1);"
        "}"
    );

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(controlBar);
    layout->addWidget(lrc);
    layout->setContentsMargins(10, 5, 10, 10);
    layout->setSpacing(5);
    setLayout(layout);

    // 设置整体窗口效果
    this->setStyleSheet(
        "DeskLrcWidget {"
        "    background: transparent;"
        "}"
    );

    connect(controlBar, &MiniControlBar::signal_close_clicked, this, &DeskLrcWidget::hide);
    connect(controlBar, &MiniControlBar::signal_play_clicked, this, &DeskLrcWidget::signal_play_clicked);
    connect(this, &DeskLrcWidget::signal_play_Clicked, controlBar, &MiniControlBar::slot_playChanged);
    connect(controlBar, &MiniControlBar::signal_next_clicked, this, &DeskLrcWidget::signal_next_clicked);
    connect(controlBar, &MiniControlBar::signal_last_clicked, this, &DeskLrcWidget::signal_last_clicked);
    connect(controlBar, &MiniControlBar::signal_forward_clicked, this, &DeskLrcWidget::signal_forward_clicked);
    connect(controlBar, &MiniControlBar::signal_backward_clicked, this, &DeskLrcWidget::signal_backward_clicked);
    connect(controlBar, &MiniControlBar::signal_set_clicked, this, &DeskLrcWidget::slot_settings_clicked);
    
    // 加载保存的设置
    loadLrcSettings();
}

void DeskLrcWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (isInResizeArea(event->pos())) {
            // 开始拉伸
            resizing = true;
            resizeStartPos = event->globalPos();
            resizeStartSize = size();
            event->accept();
        } else {
            // 开始拖拽
            m_dragPosition = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
    }
}

void DeskLrcWidget::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        if (resizing) {
            // 处理拉伸
            QPoint delta = event->globalPos() - resizeStartPos;
            QSize newSize = resizeStartSize + QSize(delta.x(), delta.y());
            
            // 限制尺寸并确保在合理范围内
            QSize minSize = minimumSize();
            QSize maxSize = maximumSize();
            
            newSize.setWidth(qMax(minSize.width(), qMin(maxSize.width(), newSize.width())));
            newSize.setHeight(qMax(minSize.height(), qMin(maxSize.height(), newSize.height())));
            
            // 只有当尺寸实际改变时才调整
            if (newSize != size()) {
                resize(newSize);
                // 立即更新布局
                layout()->update();
            }
            event->accept();
        } else {
            // 处理拖拽
            move(event->globalPos() - m_dragPosition);
            event->accept();
        }
    } else {
        // 更新鼠标光标
        Qt::CursorShape cursor = getResizeCursor(event->pos());
        if (cursor != currentCursor) {
            currentCursor = cursor;
            setCursor(cursor);
        }
    }
}

void DeskLrcWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        resizing = false;
        event->accept();
    }
}

void DeskLrcWidget::slot_receive_lrc(const QString lrc_){
    lrc->setText(lrc_);
}

void DeskLrcWidget::slot_settings_clicked()
{
    if (!settingsDialog) {
        settingsDialog = new DeskLrcSettings(this);
        connect(settingsDialog, &DeskLrcSettings::settingsChanged, 
                this, &DeskLrcWidget::slot_settings_changed);
    }
    
    // 从当前歌词控件获取实际设置
    QFont currentFont = lrc->font();
    QString styleSheet = lrc->styleSheet();
    
    // 从样式表中解析颜色
    QColor currentColor = QColor(0, 120, 255); // 默认颜色
    if (styleSheet.contains("color:")) {
        int start = styleSheet.indexOf("color:") + 6;
        int end = styleSheet.indexOf(";", start);
        if (end == -1) end = styleSheet.length();
        QString colorStr = styleSheet.mid(start, end - start).trimmed();
        currentColor = QColor(colorStr);
    }
    
    // 将当前设置设置到对话框
    settingsDialog->setLrcColor(currentColor);
    settingsDialog->setLrcFont(currentFont);
    
    settingsDialog->show();
    settingsDialog->raise();
    settingsDialog->activateWindow();
}

void DeskLrcWidget::slot_settings_changed(const QColor &color, int fontSize, const QFont &font)
{
    // 更新歌词样式 - 保持美化效果
    QFont newFont = font;
    newFont.setPointSize(fontSize);
    newFont.setWeight(QFont::Bold);
    lrc->setFont(newFont);
    
    QString styleSheet = QString(
        "QLabel {"
        "    color: %1;"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                              stop:0 rgba(0, 0, 0, 100), "
        "                              stop:1 rgba(0, 0, 0, 150));"
        "    border-radius: 20px;"
        "    padding: 15px 25px;"
        "    border: 2px solid rgba(255, 255, 255, 0.1);"
        "}"
    ).arg(color.name());
    
    lrc->setStyleSheet(styleSheet);
    
    // 保存设置到配置文件
    QSettings settings;
    settings.setValue("DeskLrc/color", color);
    settings.setValue("DeskLrc/fontSize", fontSize);
    settings.setValue("DeskLrc/fontFamily", font.family());
}

void DeskLrcWidget::loadLrcSettings()
{
    QSettings settings;
    QColor color = settings.value("DeskLrc/color", QColor(255, 255, 255)).value<QColor>();
    int fontSize = settings.value("DeskLrc/fontSize", 18).toInt();
    QString fontFamily = settings.value("DeskLrc/fontFamily", "Microsoft YaHei").toString();
    
    QFont font;
    font.setFamily(fontFamily);
    font.setPointSize(fontSize);
    font.setWeight(QFont::Bold);
    lrc->setFont(font);
    
    // 保持美化样式的同时应用用户颜色设置
    QString styleSheet = QString(
        "QLabel {"
        "    color: %1;"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                              stop:0 rgba(0, 0, 0, 100), "
        "                              stop:1 rgba(0, 0, 0, 150));"
        "    border-radius: 20px;"
        "    padding: 15px 25px;"
        "    border: 2px solid rgba(255, 255, 255, 0.1);"
        "}"
    ).arg(color.name());
    
    lrc->setStyleSheet(styleSheet);
}

void DeskLrcWidget::applyLrcSettings()
{
    loadLrcSettings();
}

Qt::CursorShape DeskLrcWidget::getResizeCursor(const QPoint &pos) {
    const int resizeMargin = 10;
    QRect rect = this->rect();
    
    bool atRight = pos.x() >= rect.width() - resizeMargin;
    bool atBottom = pos.y() >= rect.height() - resizeMargin;
    
    if (atRight && atBottom) {
        return Qt::SizeFDiagCursor;  // 右下角拉伸
    } else if (atRight) {
        return Qt::SizeHorCursor;   // 右边拉伸
    } else if (atBottom) {
        return Qt::SizeVerCursor;   // 下边拉伸
    }
    
    return Qt::ArrowCursor;
}

bool DeskLrcWidget::isInResizeArea(const QPoint &pos) {
    const int resizeMargin = 10;
    QRect rect = this->rect();
    
    bool atRight = pos.x() >= rect.width() - resizeMargin;
    bool atBottom = pos.y() >= rect.height() - resizeMargin;
    
    return atRight || atBottom;
}

QRect DeskLrcWidget::getResizeRect() {
    const int resizeMargin = 10;
    QRect rect = this->rect();
    return QRect(rect.width() - resizeMargin, rect.height() - resizeMargin, 
                 resizeMargin, resizeMargin);
}
