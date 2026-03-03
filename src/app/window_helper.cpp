#include "window_helper.h"

WindowHelper::WindowHelper(QWidget* window, QObject* parent)
    : QObject(parent), m_window(window), m_isDragging(false)
{}

void WindowHelper::startDrag()
{
    if (m_window && !m_isDragging) {
        m_dragStartMousePos = QCursor::pos();
        m_dragStartWindowPos = m_window->pos();
        m_isDragging = true;
    }
}

void WindowHelper::drag()
{
    if (m_window && m_isDragging) {
        QPoint currentMousePos = QCursor::pos();
        QPoint delta = currentMousePos - m_dragStartMousePos;
        m_window->move(m_dragStartWindowPos + delta);
    }
}

void WindowHelper::endDrag()
{
    m_isDragging = false;
}

void WindowHelper::closeWindow()
{
    if (m_window) {
        m_window->close();
    }
}

void WindowHelper::minimizeWindow()
{
    if (m_window) {
        m_window->showMinimized();
    }
}
