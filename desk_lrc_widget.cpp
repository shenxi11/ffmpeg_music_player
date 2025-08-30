#include "desk_lrc_widget.h"

DeskLrcWidget::DeskLrcWidget(QWidget* parent)
    :QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(400, 150);
    controlBar = new MiniControlBar(this);
    controlBar->setFixedHeight(30);
    controlBar->hide();

    lrc = new QLabel("暂无歌词",this);
    lrc->setAlignment(Qt::AlignCenter);
    QFont font;
    font.setFamily("Arial");
    font.setPointSize(fontSize_);
    lrc->setStyleSheet("color: blue;");
    lrc->setFont(font);

    settingWidget = new SettingWidget();
    settingWidget->setFixedWidth(150);
    settingWidget->close();

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(controlBar);
    layout->addWidget(lrc);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setLayout(layout);

    connect(controlBar, &MiniControlBar::signal_close_clicked, this, &DeskLrcWidget::hide);
    connect(controlBar, &MiniControlBar::signal_play_clicked, this, &DeskLrcWidget::signal_play_clicked);
    connect(this, &DeskLrcWidget::signal_play_Clicked, controlBar, &MiniControlBar::slot_playChanged);
    connect(controlBar, &MiniControlBar::signal_next_clicked, this, &DeskLrcWidget::signal_next_clicked);
    connect(controlBar, &MiniControlBar::signal_last_clicked, this, &DeskLrcWidget::signal_last_clicked);
    connect(controlBar, &MiniControlBar::signal_forward_clicked, this, &DeskLrcWidget::signal_forward_clicked);
    connect(controlBar, &MiniControlBar::signal_backward_clicked, this, &DeskLrcWidget::signal_backward_clicked);
    connect(controlBar,&MiniControlBar::signal_set_toggled, this, &DeskLrcWidget::slot_set_toggled);
    connect(settingWidget, &SettingWidget::signal_changeColor, this, &DeskLrcWidget::slot_changeColor);
    connect(settingWidget, &SettingWidget::signal_add_fontSize, this, &DeskLrcWidget::slot_add_fontSize);
    connect(settingWidget, &SettingWidget::signal_desc_fontSize, this, &DeskLrcWidget::slot_desc_fontSize);
}
void DeskLrcWidget::slot_add_fontSize(){
    QFont font;
    font.setFamily("Arial");
    font.setPointSize(fontSize_ + 1);
    fontSize_ += 1;
    lrc->setFont(font);
}
void DeskLrcWidget::slot_desc_fontSize(){
    QFont font;
    font.setFamily("Arial");
    font.setPointSize(std::max(fontSize_ - 1, 1));
    fontSize_ = std::max(fontSize_ - 1, 1);
    lrc->setFont(font);
}
void DeskLrcWidget::slot_set_toggled(bool checked){
    if(checked){
        settingWidget->show();
        QPoint pos_ = QCursor::pos();
        settingWidget->move(QPoint(pos_.x() + 10, pos_.y() + 10));
        settingWidget->raise();
        this->stackUnder(settingWidget);
    }
    else{
        settingWidget->close();
    }
}
void DeskLrcWidget::slot_changeColor(QString setting_){
    lrc->setStyleSheet(setting_);
}
void DeskLrcWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}
void DeskLrcWidget::slot_receive_lrc(const QString lrc_){
    lrc->setText(lrc_);
}
void DeskLrcWidget::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}
