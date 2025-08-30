#include "setting_widget.h"

SettingWidget::SettingWidget(QWidget* parent)
    :QWidget(parent)
{

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    QWidget* widget = new QWidget(this);
    font_ = new QPushButton("字体大小", widget);
    font_->setFixedHeight(50);
    font_->setFlat(true);
    font_->setStyleSheet("QPushButton {"
                         "   text-align: left;"
                         "   border: 1px solid #808080;"
                         "   padding-left: 5px;"
                         "}");
    QWidget* widget_1 = new QWidget(this);
    QHBoxLayout* layout_1 = new QHBoxLayout(this);
    add_fontSize = new QPushButton(widget_1);
    desc_fontSize = new QPushButton(widget_1);
    add_fontSize->setStyleSheet(
                "QPushButton {"
                " border-image: url(:/new/prefix1/icon/加.png);"
                "}"
                );
    desc_fontSize->setStyleSheet(
                "QPushButton {"
                " border-image: url(:/new/prefix1/icon/减.png);"
                "}"
                );
    layout_1->addWidget(add_fontSize);
    layout_1->addWidget(desc_fontSize);
    layout_1->setContentsMargins(0, 0, 0, 0);
    layout_1->setSpacing(0);
    widget_1->setLayout(layout_1);
    QHBoxLayout* hlayout = new QHBoxLayout(this);
    hlayout->addWidget(font_);
    hlayout->addWidget(widget_1);
    hlayout->setContentsMargins(0, 0, 0, 0);
    hlayout->setSpacing(10);
    widget->setLayout(hlayout);

    fontColor_ = new QPushButton("字体颜色", this);
    fontColor_->setFixedHeight(50);
    fontColor_->setCheckable(true);
    fontColor_->setFlat(true);
    fontColor_->setStyleSheet("QPushButton {"
                              "   text-align: left;"
                              "   border: 1px solid #808080;"
                              "   padding-left: 5px;"
                              "}");
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(widget);
    layout->addWidget(fontColor_);
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
    fontColor_widget->setFixedWidth(100);
    QVBoxLayout* fontColor_layout = new QVBoxLayout(fontColor_widget);
    for(auto string_: fontColorList_){
        QPushButton* button = new QPushButton(string_, fontColor_widget);
        connect(button, &QPushButton::clicked, this, [=](){
            slot_changeColor(string_);
        });
        fontColor_layout->setContentsMargins(0, 0, 0, 0);
        fontColor_layout->setSpacing(0);
        button->setFixedHeight(40);
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
