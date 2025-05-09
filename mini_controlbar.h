#ifndef MINICONTROLBAR_H
#define MINICONTROLBAR_H

#include <QObject>
#include <QWidget>
#include <QEnterEvent>
#include <QHoverEvent>
#include <QScreen>
#include <QApplication>

#include "controlbar.h"
#include "headers.h"
class Button : public QPushButton
{
public:
    Button(QString text, QPushButton* parent = nullptr)
        :text_(text),
          QPushButton(parent)
    {
        setAttribute( Qt::WA_Hover,true);
        info_ = new QLabel(text_);
        info_->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        info_->setFixedHeight(20);
        info_->close();
    }
    ~Button(){
        info_->deleteLater();
    }
protected:
    bool event(QEvent* event) {
        if (event->type() == QEvent::HoverEnter) {
            auto mousePos = QCursor::pos();
            auto labelPos = QPoint(mousePos.x(), mousePos.y() + 10);
            info_->show();
            info_->move(labelPos);
            info_->raise();
        } else if (event->type() == QEvent::HoverLeave || event->type() == QEvent::Leave) {
            info_->close();
        }
        return QWidget::event(event);
    }
private:
    QLabel* info_;
    QString text_;
};

class MiniControlBar : public QWidget
{
    Q_OBJECT
public:
    MiniControlBar(QWidget* parent = nullptr);
signals:
    void signal_next_clicked();
    void signal_last_clicked();
    void signal_forward_clicked();
    void signal_backward_clicked();
    void signal_set_clicked();
    void signal_close_clicked();
    void signal_lock_clicked();
    void signal_play_clicked();
public slots:
    void slot_playChanged(ControlBar::State);
private:
    Button* play_;
    Button* next_;
    Button* last_;
    Button* forward_;
    Button* backWard_;
    Button* set_;
    Button* close_;
    Button* lock_;
};

#endif // MINICONTROLBAR_H
