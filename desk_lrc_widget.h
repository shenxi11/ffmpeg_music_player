#ifndef DESKLRCWIDGET_H
#define DESKLRCWIDGET_H

#include <QObject>
#include <QWidget>
#include "headers.h"
#include "mini_controlbar.h"
#include "setting_widget.h"

class DeskLrcWidget : public QWidget
{
    Q_OBJECT
public:
    DeskLrcWidget(QWidget* parent = nullptr);
signals:
    void signal_play_clicked();
    void signal_play_Clicked(ControlBar::State);
    void signal_next_clicked();
    void signal_last_clicked();
    void signal_forward_clicked();
    void signal_backward_clicked();
public slots:
    void slot_receive_lrc(const QString lrc_);
    void slot_changeColor(QString setting_);
    void slot_set_toggled(bool checked);
    void slot_add_fontSize();
    void slot_desc_fontSize();
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override{
        hovered = true;
        controlBar->show();
    }
    void leaveEvent(QEvent *event) override{
        hovered = false;
        controlBar->hide();
    }
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);

        QPainter painter(this);
        if(hovered){
            painter.setRenderHint(QPainter::Antialiasing);

            painter.fillRect(rect(), QColor(0, 0, 0, 128));
        }
    }
    void closeEvent(QCloseEvent *event) override{
        if(settingWidget)
            settingWidget->close();
    }
private:
    QLabel* lrc;
    MiniControlBar* controlBar;
    SettingWidget* settingWidget;
    QPoint m_dragPosition;

    bool hovered = false;
    int fontSize_ = 20;
};

#endif // DESKLRCWIDGET_H
