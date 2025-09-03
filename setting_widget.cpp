#include "setting_widget.h"

SettingWidget::SettingWidget(QWidget* parent)
    :QWidget(parent)
{

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setStyleSheet("QWidget#SettingWidget{background:#fff; border-radius:18px;} QPushButton{border-radius:12px; font-size:15px; background:#f7f9fc; color:#222;} QPushButton:hover{background:#eafff3; color:#1DB954;}");
    this->setObjectName("SettingWidget");

    // 字体大小设置
    QWidget* fontGroup = new QWidget(this);
    QHBoxLayout* fontGroupLayout = new QHBoxLayout(fontGroup);
    fontGroupLayout->setContentsMargins(18, 18, 18, 0);
    fontGroupLayout->setSpacing(12);
    font_ = new QPushButton("字体大小", fontGroup);
    font_->setFixedHeight(40);
    font_->setFlat(true);
    font_->setStyleSheet("QPushButton{background:transparent; text-align:left; font-weight:bold;}");
    add_fontSize = new QPushButton(fontGroup);
    desc_fontSize = new QPushButton(fontGroup);
    add_fontSize->setFixedSize(32,32);
    desc_fontSize->setFixedSize(32,32);
    add_fontSize->setStyleSheet("QPushButton{border:none; background:transparent; border-image: url(:/new/prefix1/icon/加.png);}");
    desc_fontSize->setStyleSheet("QPushButton{border:none; background:transparent; border-image: url(:/new/prefix1/icon/减.png);}");
    fontGroupLayout->addWidget(font_);
    fontGroupLayout->addStretch();
    fontGroupLayout->addWidget(desc_fontSize);
    fontGroupLayout->addWidget(add_fontSize);
    fontGroup->setLayout(fontGroupLayout);

    // 字体颜色设置
    fontColor_ = new QPushButton("字体颜色", this);
    fontColor_->setFixedHeight(40);
    fontColor_->setCheckable(true);
    fontColor_->setFlat(true);
    fontColor_->setStyleSheet("QPushButton{background:transparent; text-align:left; font-weight:bold;}");

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(fontGroup);
    layout->addWidget(fontColor_);
    layout->addStretch();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setLayout(layout);

    fontColorList_<<"清新绿"
                 <<"可爱粉"
                <<"深邃蓝"
               <<"高雅灰"
              <<"活力黄";
    colorMap[fontColorList_.at(0)] = "color: lime";
    colorMap[fontColorList_.at(1)] = "color: hotpink";
    colorMap[fontColorList_.at(2)] = "color: darkblue";
    colorMap[fontColorList_.at(3)] = "color: gray";
    colorMap[fontColorList_.at(4)] = "color: yellow";
    fontColor_widget = new QWidget();
    fontColor_widget->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    fontColor_widget->setFixedWidth(120);
    fontColor_widget->setStyleSheet("QWidget{background:#fff; border-radius:12px; border:1px solid #e0e0e0;}");
    QVBoxLayout* fontColor_layout = new QVBoxLayout(fontColor_widget);
    fontColor_layout->setContentsMargins(0, 0, 0, 0);
    fontColor_layout->setSpacing(0);
    for(auto string_: fontColorList_){
        QPushButton* button = new QPushButton(string_, fontColor_widget);
        button->setStyleSheet("QPushButton{border:none; background:transparent; font-size:14px;} QPushButton:hover{background:#eafff3; color:#1DB954;}");
        connect(button, &QPushButton::clicked, this, [=](){
            slot_changeColor(string_);
        });
        button->setFixedHeight(36);
        fontColor_layout->addWidget(button);
    }
    fontColor_widget->setLayout(fontColor_layout);
    fontColor_widget->close();

    connect(fontColor_, &QPushButton::toggled, this, &SettingWidget::slot_fontColor_clicked);
    connect(add_fontSize, &QPushButton::clicked, this, &SettingWidget::signal_add_fontSize);
    connect(desc_fontSize, &QPushButton::clicked, this, &SettingWidget::signal_desc_fontSize);
}
void SettingWidget::slot_fontColor_clicked(bool checked){
    if(checked){
        fontColor_widget->show();
        QPoint global = frameGeometry().topLeft();
        fontColor_widget->move(QPoint(global.x() + 150, global.y() + 50));
        fontColor_widget->raise();
    }else{
        fontColor_widget->close();
    }
}
void SettingWidget::slot_changeColor(QString color){
    emit signal_changeColor(colorMap[color]);
    fontColor_widget->close();
    fontColor_->setChecked(false);
}
