#ifndef PROCESSSLIDER_H
#define PROCESSSLIDER_H

#include <QObject>
#include <QSlider>
#include <QScrollBar>
#include <QLabel>
#include <QWidget>
#include <QHBoxLayout>

class ProcessSlider : public QWidget
{
    Q_OBJECT
public:
    ProcessSlider(QWidget *parent = nullptr);

    void setMaxSeconds(int seconds_);
    void setValueOverride(int value);
    int value();
    int maxValue();

    void slot_isUpChanged(bool flag);
    void slot_forward();
    void slot_backward();
signals:
    void signal_sliderPressed();
    void signal_sliderReleased();
    void signal_playFinished();
private slots:
    void slot_valueChanged(int value);
private:
    void update_time();
    int seconds_ = 0;
    QSlider* slider;
    QLabel* endTime_label;
    QLabel* startTime_label;

};

#endif // PROCESSSLIDER_H
