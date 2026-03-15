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

    void onIsUpChanged(bool flag);
    void onForward();
    void onBackward();
    void onDurationChanged(qint64 value);
    int getPressPosition() { return press_position; }
    void setPrintF(bool flag) { printF = flag; };
signals:
    void signalSliderPressed();
    void signalSliderReleased();
    void signalPlayFinished();
private slots:
    void onSliderPressed();
    void onValueChanged(int value);
private:
    void updateTime();
    int seconds_ = 0;
    QSlider* slider;
    QLabel* endTime_label;
    QLabel* startTime_label;

    int press_position = 0;
    bool printF = false;
};

#endif // PROCESSSLIDER_H
