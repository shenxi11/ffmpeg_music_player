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

    play_->setFixedSize(20, 20);
    next_->setFixedSize(20, 20);
    last_->setFixedSize(20, 20);
    forward_->setFixedSize(20, 20);
    backWard_->setFixedSize(20, 20);
    set_->setFixedSize(20, 20);
    close_->setFixedSize(20, 20);
    lock_->setFixedSize(20, 20);

    play_->setStyleSheet("QPushButton {"
                         "    border-image: url(:/new/prefix1/icon/play.png);"
                         "}");
    last_->setStyleSheet("QPushButton {"
                         "    border-image: url(:/new/prefix1/icon/last_song.png);"
                         "}");
    next_->setStyleSheet("QPushButton {"
                         "    border-image: url(:/new/prefix1/icon/next_song.png);"
                         "}");
    forward_->setStyleSheet("QPushButton {"
                         "    border-image: url(:/new/prefix1/icon/快进右.png);"
                         "}");
    backWard_->setStyleSheet("QPushButton {"
                         "    border-image: url(:/new/prefix1/icon/快进左.png);"
                         "}");
    set_->setStyleSheet("QPushButton {"
                         "    border-image: url(:/new/prefix1/icon/设置.png);"
                         "}");
    close_->setStyleSheet("QPushButton {"
                         "    border-image: url(:/new/prefix1/icon/关闭.png);"
                         "}");
    lock_->setStyleSheet("QPushButton {"
                         "    border-image: url(:/new/prefix1/icon/锁定_o.png);"
                         "}");

    connect(close_, &Button::clicked, this, &MiniControlBar::signal_close_clicked);
    connect(play_, &Button::clicked, this, &MiniControlBar::signal_play_clicked);
    connect(next_, &Button::clicked, this, &MiniControlBar::signal_next_clicked);
    connect(last_, &Button::clicked, this, &MiniControlBar::signal_last_clicked);
    connect(forward_, &Button::clicked, this, &MiniControlBar::signal_forward_clicked);
    connect(backWard_, &Button::clicked, this, &MiniControlBar::signal_backward_clicked);

}
void MiniControlBar::slot_playChanged(ControlBar::State state){
    if(state == ControlBar::Play){
        play_->setStyleSheet("QPushButton {"
                             "    border-image: url(:/new/prefix1/icon/pause.png);"
                             "}");

    }else{
        play_->setStyleSheet("QPushButton {"
                             "    border-image: url(:/new/prefix1/icon/play.png);"
                             "}");
    }
}
