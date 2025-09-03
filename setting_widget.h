#ifndef SETTINGWIDGET_H
#define SETTINGWIDGET_H

#include <QObject>
#include <QWidget>
#include <QMenu>
#include <QWidgetAction>
#include "headers.h"

class SettingWidget : public QWidget
{
    Q_OBJECT
public:
    SettingWidget(QWidget* parent = nullptr);
protected:
    void closeEvent(QCloseEvent *event) override{
        if(fontColor_widget)
            fontColor_widget->close();
    }
signals:
    void signal_changeColor(QString settings_);
    void signal_add_fontSize();
    void signal_desc_fontSize();
private:
    void slot_fontColor_clicked(bool checked);
    void slot_changeColor(QString color);
private:
    // 字体大小设置
    QPushButton* font_;
    QPushButton* add_fontSize;
    QPushButton* desc_fontSize;

    // 字体颜色设置
    QPushButton* fontColor_;
    QWidget* fontColor_widget;
    QStringList fontColorList_;
    QMap<QString, QString> colorMap;
};

#endif // SETTINGWIDGET_H
