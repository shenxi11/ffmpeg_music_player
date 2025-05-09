#ifndef CONTROLBAR_H
#define CONTROLBAR_H

#include <QObject>
#include <QSlider>
#include "headers.h"

class ControlBar : public QWidget
{
    Q_OBJECT
public:
    enum State{
      Stop,
      Play,
      Pause
    };

    ControlBar(QWidget *parent = nullptr);
    State getState();
    bool getLoopFlag();
public slots:
    void slot_playState(State state_);
    void slot_playFinished();
    void slot_isUpChanged(bool flag);
signals:
    void signal_stop();
    void signal_nextSong();
    void signal_lastSong();
    void signal_volumeChanged(int value);
    void signal_mlist_toggled(bool checked);
    void signal_play_clicked();
    void signal_rePlay();
    void signal_desk_toggled(bool checked);
private slots:
    void slot_loop_clicked();
    void slot_volume_toggled(bool checked);
private:
    QPushButton *last;
    QPushButton *next;
    QPushButton *play;
    QPushButton *loop;
    QPushButton *volume;
    QPushButton *mlist;
    QPushButton *desk;

    QSlider* volumeSlider;

    State state_ = Stop;
    bool loopState_ = false;
    bool isUp = false;
};

#endif // CONTROLBAR_H
