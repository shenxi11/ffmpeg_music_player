#ifndef WINDOW_HELPER_H
#define WINDOW_HELPER_H

#include <QObject>
#include <QWidget>
#include <QPoint>
#include <QCursor>

class WindowHelper : public QObject
{
    Q_OBJECT
    
public:
    explicit WindowHelper(QWidget* window, QObject* parent = nullptr)
        : QObject(parent), m_window(window), m_isDragging(false) {}
    
    Q_INVOKABLE void startDrag() {
        if (m_window && !m_isDragging) {
            m_dragStartMousePos = QCursor::pos();
            m_dragStartWindowPos = m_window->pos();
            m_isDragging = true;
        }
    }
    
    Q_INVOKABLE void drag() {
        if (m_window && m_isDragging) {
            QPoint currentMousePos = QCursor::pos();
            QPoint delta = currentMousePos - m_dragStartMousePos;
            m_window->move(m_dragStartWindowPos + delta);
        }
    }
    
    Q_INVOKABLE void endDrag() {
        m_isDragging = false;
    }
    
    Q_INVOKABLE void closeWindow() {
        if (m_window) {
            m_window->close();
        }
    }
    
    Q_INVOKABLE void minimizeWindow() {
        if (m_window) {
            m_window->showMinimized();
        }
    }
    
private:
    QWidget* m_window;
    QPoint m_dragStartMousePos;
    QPoint m_dragStartWindowPos;
    bool m_isDragging;
};

#endif // WINDOW_HELPER_H
