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
        info_->setFixedHeight(25);
        
        // 美化工具提示样式
        info_->setStyleSheet(
            "QLabel {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
            "                              stop:0 #4a4a4a, stop:1 #2a2a2a);"
            "    color: white;"
            "    border: 1px solid #666;"
            "    border-radius: 8px;"
            "    padding: 4px 8px;"
            "    font-size: 11px;"
            "    font-weight: bold;"
            "}"
        );
        
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
