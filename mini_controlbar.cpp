#include "mini_controlbar.h"

MiniControlBar::MiniControlBar(QWidget* parent)
    :QWidget(parent)
{
    play_ = new Button("播放");
    next_= new Button("下一首");
    last_= new Button("上一首");
    forward_= new Button("前进1秒");
    backWard_= new Button("后退一秒");
    set_= new Button("设置");
    close_= new Button("关闭");
    lock_= new Button("锁定");

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(last_);
    layout->addWidget(play_);
    layout->addWidget(next_);
    layout->addWidget(backWard_);
    layout->addWidget(forward_);
    layout->addWidget(lock_);
    layout->addWidget(set_);
    layout->addWidget(close_);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setLayout(layout);

    play_->setFixedSize(28, 28);
    next_->setFixedSize(28, 28);
    last_->setFixedSize(28, 28);
    forward_->setFixedSize(28, 28);
    backWard_->setFixedSize(28, 28);
    set_->setFixedSize(28, 28);
    close_->setFixedSize(28, 28);
    lock_->setFixedSize(28, 28);

    // 现代化按钮样式
    QString buttonStyle = 
        "QPushButton {"
        "    border: none;"
        "    border-radius: 14px;"
        "    background: rgba(255, 255, 255, 0.1);"
        "    padding: 4px;"
        "}"
        "QPushButton:hover {"
        "    background: rgba(102, 126, 234, 0.3);"
        "    border: 1px solid rgba(102, 126, 234, 0.5);"
        "}"
        "QPushButton:pressed {"
        "    background: rgba(102, 126, 234, 0.5);"
        "    border: 1px solid rgba(102, 126, 234, 0.8);"
        "}";

    play_->setStyleSheet(buttonStyle + "QPushButton { border-image: url(:/new/prefix1/icon/play.png); }");
    last_->setStyleSheet(buttonStyle + "QPushButton { border-image: url(:/new/prefix1/icon/last_song.png); }");
    next_->setStyleSheet(buttonStyle + "QPushButton { border-image: url(:/new/prefix1/icon/next_song.png); }");
    forward_->setStyleSheet(buttonStyle + "QPushButton { border-image: url(:/new/prefix1/icon/fast_forward_right.png); }");
    backWard_->setStyleSheet(buttonStyle + "QPushButton { border-image: url(:/new/prefix1/icon/fast_forward_left.png); }");
    set_->setStyleSheet(buttonStyle + "QPushButton { border-image: url(:/new/prefix1/icon/settings.png); }");
    close_->setStyleSheet(buttonStyle + "QPushButton { border-image: url(:/new/prefix1/icon/close.png); }");
    lock_->setStyleSheet(buttonStyle + "QPushButton { border-image: url(:/new/prefix1/icon/lock_o.png); }");

    connect(close_, &Button::clicked, this, &MiniControlBar::signal_close_clicked);
    connect(play_, &Button::clicked, this, &MiniControlBar::signal_play_clicked);
    connect(next_, &Button::clicked, this, &MiniControlBar::signal_next_clicked);
    connect(last_, &Button::clicked, this, &MiniControlBar::signal_last_clicked);
    connect(forward_, &Button::clicked, this, &MiniControlBar::signal_forward_clicked);
    connect(backWard_, &Button::clicked, this, &MiniControlBar::signal_backward_clicked);
    connect(set_, &Button::clicked, this, &MiniControlBar::signal_set_clicked);

}
void MiniControlBar::slot_playChanged(ControlBar::State state){
    QString buttonStyle = 
        "QPushButton {"
        "    border: none;"
        "    border-radius: 14px;"
        "    background: rgba(255, 255, 255, 0.1);"
        "    padding: 4px;"
        "}"
        "QPushButton:hover {"
        "    background: rgba(102, 126, 234, 0.3);"
        "    border: 1px solid rgba(102, 126, 234, 0.5);"
        "}"
        "QPushButton:pressed {"
        "    background: rgba(102, 126, 234, 0.5);"
        "    border: 1px solid rgba(102, 126, 234, 0.8);"
        "}";

    if(state == ControlBar::Play){
        play_->setStyleSheet(buttonStyle + "QPushButton { border-image: url(:/new/prefix1/icon/pause.png); }");
    }else{
        play_->setStyleSheet(buttonStyle + "QPushButton { border-image: url(:/new/prefix1/icon/play.png); }");
    }
}
