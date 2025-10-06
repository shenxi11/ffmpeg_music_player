#ifndef DESKLRCWIDGET_H
#define DESKLRCWIDGET_H

#include <QObject>
#include <QWidget>
#include <QSettings>
#include "headers.h"
#include "mini_controlbar.h"
#include "desk_lrc_settings.h"

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
    void slot_settings_clicked();
    void slot_settings_changed(const QColor &color, int fontSize, const QFont &font);
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override{
        hovered = true;
        controlBar->show();
        update(); // 触发重绘
    }
    void leaveEvent(QEvent *event) override{
        hovered = false;
        controlBar->hide();
        update(); // 触发重绘
    }
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        if(hovered){
            // 绘制现代化的悬停背景
            QRect bgRect = rect().adjusted(5, 2, -5, -5);
            
            // 创建渐变背景
            QLinearGradient gradient(0, 0, 0, height());
            gradient.setColorAt(0, QColor(40, 40, 40, 180));
            gradient.setColorAt(0.5, QColor(60, 60, 60, 160));
            gradient.setColorAt(1, QColor(20, 20, 20, 200));
            
            painter.setBrush(QBrush(gradient));
            painter.setPen(QPen(QColor(255, 255, 255, 50), 1));
            painter.drawRoundedRect(bgRect, 15, 15);
            
            // 添加内发光效果
            QPen glowPen(QColor(102, 126, 234, 100), 2);
            painter.setPen(glowPen);
            painter.drawRoundedRect(bgRect.adjusted(1, 1, -1, -1), 14, 14);
            
            // 绘制拉伸提示
            const int resizeMargin = 10;
            QRect resizeArea(width() - resizeMargin, height() - resizeMargin, 
                           resizeMargin, resizeMargin);
            
            painter.setPen(QPen(QColor(102, 126, 234, 150), 2));
            painter.setBrush(QColor(102, 126, 234, 50));
            painter.drawRect(resizeArea.adjusted(-2, -2, 0, 0));
            
            // 绘制拉伸手柄线条
            painter.setPen(QPen(QColor(255, 255, 255, 200), 1));
            for (int i = 0; i < 3; ++i) {
                int offset = i * 3 + 2;
                painter.drawLine(width() - resizeMargin + offset, height() - 2,
                               width() - 2, height() - resizeMargin + offset);
            }
        }
    }

private:
    QLabel* lrc;
    MiniControlBar* controlBar;
    QPoint m_dragPosition;
    DeskLrcSettings* settingsDialog;

    bool hovered = false;
    bool resizing = false;
    QPoint resizeStartPos;
    QSize resizeStartSize;
    Qt::CursorShape currentCursor = Qt::ArrowCursor;
    
    void loadLrcSettings();
    void applyLrcSettings();
    Qt::CursorShape getResizeCursor(const QPoint &pos);
    bool isInResizeArea(const QPoint &pos);
    QRect getResizeRect();
};

#endif // DESKLRCWIDGET_H
