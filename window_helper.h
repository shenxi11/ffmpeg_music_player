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
    explicit WindowHelper(QWidget* window, QObject* parent = nullptr);
    
    Q_INVOKABLE void startDrag();
    Q_INVOKABLE void drag();
    Q_INVOKABLE void endDrag();
    Q_INVOKABLE void closeWindow();
    Q_INVOKABLE void minimizeWindow();
    
private:
    QWidget* m_window;
    QPoint m_dragStartMousePos;
    QPoint m_dragStartWindowPos;
    bool m_isDragging;
};

#endif // WINDOW_HELPER_H
