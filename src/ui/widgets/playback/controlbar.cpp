#include "controlbar.h"

ControlBar::ControlBar(QWidget *parent)
    :QWidget(parent)
{
    volumeSlider = new QSlider(Qt::Horizontal, this);
    volumeSlider->hide();
    volumeSlider->setMinimum(0);
    volumeSlider->setMaximum(100);
    volumeSlider->setValue(50);
    volumeSlider->setTickPosition(QSlider::TicksBelow);
    volumeSlider->setTickInterval(10);
    volumeSlider->resize(100, 30);

    play  = new QPushButton(this);
    volume = new QPushButton(this);
    loop  = new QPushButton(this);
    mlist = new QPushButton(this);
    last  = new QPushButton(this);
    next  = new QPushButton(this);
    desk = new QPushButton(this);

    int height = 70;
    int width = 70;

    loop->setFixedSize(width,height);
    last->setFixedSize(width,height);
    play->setFixedSize(width,height);
    next->setFixedSize(width,height);
    volume->setFixedSize(width,height);
    mlist->setFixedSize(width,height);
    desk->setFixedSize(width,height);

    QHBoxLayout* hlayout = new QHBoxLayout(this);
    hlayout->addWidget(loop);
    hlayout->addWidget(last);
    hlayout->addWidget(play);
    hlayout->addWidget(next);
    hlayout->addWidget(volume);
    hlayout->addWidget(desk);
    hlayout->addWidget(mlist);
    hlayout->setContentsMargins(0, 0, 0, 0);
    hlayout->setSpacing(0);
    setLayout(hlayout);

    volume->setCheckable(true);
    mlist->setCheckable(true);
    desk->setCheckable(true);

    volume->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/volume.png);"
                "}"
                );

    play->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/play.png);"
                "}"
                );
    mlist->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/playlist.png);"
                "}"
                );
    last->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/last_song.png);"
                "}"
                );
    next->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/next_song.png);"
                "}"
                );
    loop->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/random_play.png);"
                "}"
                );
    desk->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/vocabulary_rights.png);"
                "}"
                );

    connect(next, &QPushButton::clicked, this, &ControlBar::signalNextSong);
    connect(last, &QPushButton::clicked, this, &ControlBar::signalLastSong);
    connect(loop, &QPushButton::clicked, this, &ControlBar::onLoopClicked);
    connect(volume, &QPushButton::toggled, this, &ControlBar::onVolumeToggled);
    connect(volumeSlider, &QSlider::valueChanged, this, &ControlBar::signalVolumeChanged);
    connect(mlist, &QPushButton::toggled, this, &ControlBar::signalMlistToggled);
    connect(play, &QPushButton::clicked, this, &ControlBar::signalPlayClicked);
    connect(desk, &QPushButton::toggled, this, &ControlBar::signalDeskToggled);
}
void ControlBar::onPlayFinished(){
    if(loopState_){
        emit signalRePlay();
    }else{
        onPlayState(Stop);
        emit signalStop();
    }
}
bool ControlBar::getLoopFlag(){
    return loopState_;
}
void ControlBar::onPlayState(State state){
    if(state == State::Play){
        if(!isUp)
            play->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/pause.png);"
                        "}"
                        );
        else
            play->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/pause_w.png);"
                        "}"
                        );
    }else {
        if(!isUp)
            play->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/play.png);"
                        "}"
                        );
        else
            play->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/play_w.png);"
                        "}"
                        );
    }
    state_ = state;
}
void ControlBar::onIsUpChanged(bool flag){
    isUp = flag;
    if(isUp){
        volume->setStyleSheet(
                    "QPushButton {"
                    "    border-image: url(:/new/prefix1/icon/volume_w.png);"
                    "}"
                    );
        if(state_ == Play)
            play->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/pause_w.png);"
                        "}"
                        );
        else
            play->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/play_w.png);"
                        "}"
                        );
        mlist->setStyleSheet(
                    "QPushButton {"
                    "    border-image: url(:/new/prefix1/icon/playlist_w.png);"
                    "}"
                    );
        last->setStyleSheet(
                    "QPushButton {"
                    "    border-image: url(:/new/prefix1/icon/last_song_w.png);"
                    "}"
                    );
        next->setStyleSheet(
                    "QPushButton {"
                    "    border-image: url(:/new/prefix1/icon/next_song_w.png);"
                    "}"
                    );
        if(loopState_)
            loop->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/loop_w.png);"
                        "}"
                        );
        else
            loop->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/random_play_w.png);"
                        "}"
                        );
    }else{
        volume->setStyleSheet(
                    "QPushButton {"
                    "    border-image: url(:/new/prefix1/icon/volume.png);"
                    "}"
                    );
        if(state_ == Play)
            play->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/pause.png);"
                        "}"
                        );
        else
            play->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/play.png);"
                        "}"
                        );
        mlist->setStyleSheet(
                    "QPushButton {"
                    "    border-image: url(:/new/prefix1/icon/playlist.png);"
                    "}"
                    );
        last->setStyleSheet(
                    "QPushButton {"
                    "    border-image: url(:/new/prefix1/icon/last_song.png);"
                    "}"
                    );
        next->setStyleSheet(
                    "QPushButton {"
                    "    border-image: url(:/new/prefix1/icon/next_song.png);"
                    "}"
                    );
        if(loopState_)
            loop->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/loop.png);"
                        "}"
                        );
        else
            loop->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/random_play.png);"
                        "}"
                        );

    }
}
void ControlBar::onVolumeToggled(bool checked){
    if(checked)
    {
        auto Position = volume->pos();
        volumeSlider->move(Position.x(), Position.y() + 20);
        volumeSlider->show();
        volumeSlider->raise();
    }
    else
    {
        if(volumeSlider)
        {
            volumeSlider->close();
        }
    }
}
void ControlBar::onLoopClicked(){
    if(loopState_){
        if(!isUp)
            loop->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/random_play.png);"
                        "}"
                        );
        else
            loop->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/random_play_w.png);"
                        "}"
                        );
    }else
    {
        if(!isUp)
            loop->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/loop.png);"
                        "}"
                        );
        else
            loop->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/loop_w.png);"
                        "}"
                        );
    }
    loopState_ = !loopState_;
}
ControlBar::State ControlBar::getState(){
    return state_;
}
