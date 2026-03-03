#include "process_slider.h"
#include <QDebug>
ProcessSlider::ProcessSlider(QWidget *parent)
    : QWidget(parent)
{
    slider = new QSlider(Qt::Horizontal,this);
    slider->setMinimum(0);
    slider->setMaximum(0);

    // 简化的进度条样式
    slider->setStyleSheet(
        "QSlider::groove:horizontal {"
        "    border: none;"
        "    background: #3d3d3d;"
        "    height: 6px;"
        "    border-radius: 3px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "                              stop:0 #667eea, stop:1 #764ba2);"
        "    border: none;"
        "    height: 6px;"
        "    border-radius: 3px;"
        "}"
        "QSlider::add-page:horizontal {"
        "    background: #4d4d4d;"
        "    border: none;"
        "    height: 6px;"
        "    border-radius: 3px;"
        "}"
        "QSlider::handle:horizontal {"
        "    background: #ffffff;"
        "    border: 2px solid #667eea;"
        "    width: 14px;"
        "    height: 14px;"
        "    margin-top: -4px;"
        "    margin-bottom: -4px;"
        "    border-radius: 7px;"
        "}"
        "QSlider::handle:horizontal:hover {"
        "    background: #f0f0f0;"
        "    border: 2px solid #764ba2;"
        "}"
        "QSlider::handle:horizontal:pressed {"
        "    background: #e0e0e0;"
        "    border: 2px solid #5a67d8;"
        "}"
    );

    startTime_label = new QLabel(this);
    startTime_label->setText("00:00");
    startTime_label->setFixedSize(50, 20);
    startTime_label->setAlignment(Qt::AlignCenter);
    startTime_label->setStyleSheet(
        "QLabel {"
        "    color: #2d3748;"
        "    font-family: 'Microsoft YaHei';"
        "    font-size: 12px;"
        "    font-weight: bold;"
        "    background: transparent;"
        "    border: none;"
        "}"
    );

    endTime_label = new QLabel(this);
    endTime_label->setText("00:00");
    endTime_label->setFixedSize(50, 20);
    endTime_label->setAlignment(Qt::AlignCenter);
    endTime_label->setStyleSheet(
        "QLabel {"
        "    color: #2d3748;"
        "    font-family: 'Microsoft YaHei';"
        "    font-size: 12px;"
        "    font-weight: bold;"
        "    background: transparent;"
        "    border: none;"
        "}"
    );

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(startTime_label);
    layout->addWidget(slider);
    layout->addWidget(endTime_label);
    layout->setContentsMargins(10, 5, 10, 5);
    layout->setSpacing(10);
    setLayout(layout);

    // 设置整体widget样式
    this->setStyleSheet(
        "ProcessSlider {"
        "    background: transparent;"
        "    border: none;"
        "}"
    );

    connect(slider, &QSlider::valueChanged, this, &ProcessSlider::slot_valueChanged);
    connect(slider, &QSlider::sliderReleased, this, &ProcessSlider::signal_sliderReleased);
    connect(slider, &QSlider::sliderPressed, this, &ProcessSlider::signal_sliderPressed);
    connect(slider, &QSlider::sliderPressed, this, [=]() {
        press_position = slider->value();
     });
}

void ProcessSlider::slot_change_duartion(qint64 value) {
    if (printF) {
        qDebug() << __FUNCTION__ << "time :" << value / 1000;
        printF = false;
    }
     setValueOverride(value / 1000);
}
int ProcessSlider::value(){
    return slider->value();
}
void ProcessSlider::slot_isUpChanged(bool flag){
    QString timeStyle;
    if(flag){
        // 浅色主题下的时间标签样式
        timeStyle = 
            "QLabel {"
            "    color: #ffffff;"
            "    font-family: 'Microsoft YaHei';"
            "    font-size: 12px;"
            "    font-weight: bold;"
            "    background: transparent;"
            "    border: none;"
            "}";
    } else {
        // 深色主题下的时间标签样式
        timeStyle = 
            "QLabel {"
            "    color: #2d3748;"
            "    font-family: 'Microsoft YaHei';"
            "    font-size: 12px;"
            "    font-weight: bold;"
            "    background: transparent;"
            "    border: none;"
            "}";
    }
    
    startTime_label->setStyleSheet(timeStyle);
    endTime_label->setStyleSheet(timeStyle);
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
