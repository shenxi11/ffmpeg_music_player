#include "process_slider.h"
#include <QDebug>
ProcessSlider::ProcessSlider(QWidget *parent)
    : QWidget(parent)
{
    slider = new QSlider(Qt::Horizontal,this);
    slider->setMinimum(0);
    slider->setMaximum(0);

    startTime_label = new QLabel(this);
    startTime_label->setText("00:00");
    startTime_label->setFixedSize(50, 20);
    startTime_label->setAlignment(Qt::AlignCenter);

    endTime_label = new QLabel(this);
    endTime_label->setText("00:00");
    endTime_label->setFixedSize(50, 20);
    endTime_label->setAlignment(Qt::AlignCenter);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(startTime_label);
    layout->addWidget(slider);
    layout->addWidget(endTime_label);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setLayout(layout);

    connect(slider, &QSlider::valueChanged, this, &ProcessSlider::slot_valueChanged);
    connect(slider, &QSlider::sliderReleased, this, &ProcessSlider::signal_sliderReleased);
    connect(slider, &QSlider::sliderPressed, this, &ProcessSlider::signal_sliderPressed);
}
int ProcessSlider::value(){
    return slider->value();
}
void ProcessSlider::slot_isUpChanged(bool flag){
    if(flag){
        startTime_label->setStyleSheet("color: white;");
        endTime_label->setStyleSheet("color: white;");
    }else{
        startTime_label->setStyleSheet("color: black;");
        endTime_label->setStyleSheet("color: black;");
    }
}
void ProcessSlider::slot_backward(){
    int value = slider->value();
    value = std::max(0, value - 1);
    slider->setValue(value);
    update_time();
}
void ProcessSlider::slot_forward(){
    int value = slider->value();
    value = std::min(seconds_, value + 1);
    slider->setValue(value);
    update_time();
}
int ProcessSlider::maxValue(){
    return seconds_;
}
void ProcessSlider::setMaxSeconds(int seconds){
    slider->setMaximum(seconds);
    seconds_ = seconds;
    update_time();
}
void ProcessSlider::setValueOverride(int value){
    if(value == slider->value())
        return;
    slider->setValue(value);
    update_time();
}
void ProcessSlider::slot_valueChanged(int value){
    update_time();
}
void ProcessSlider::update_time(){
    int time_ = slider->value();
    if(seconds_ < time_)
        return;
    int last = seconds_ - time_;
    int end_min = last / 60;
    int end_sec = last % 60;
    endTime_label->setText(QString::number(end_min).rightJustified(2, '0') + ":" + QString::number(end_sec).rightJustified(2, '0'));

    int start_min = time_ / 60;
    int start_sec = time_ % 60;
    startTime_label->setText(QString::number(start_min).rightJustified(2, '0') + ":" + QString::number(start_sec).rightJustified(2, '0'));
    if(time_ == seconds_){
        emit signal_playFinished();
    }
}
