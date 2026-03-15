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
    void onPlayState(State state_);
    void onPlayFinished();
    void onIsUpChanged(bool flag);
signals:
    void signalStop();
    void signalNextSong();
    void signalLastSong();
    void signalVolumeChanged(int value);
    void signalMlistToggled(bool checked);
    void signalPlayClicked();
    void signalRePlay();
    void signalDeskToggled(bool checked);
private slots:
    void onLoopClicked();
    void onVolumeToggled(bool checked);
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
