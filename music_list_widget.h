#ifndef MUSICLISTWIDGET_H
#define MUSICLISTWIDGET_H

#include <QObject>
#include <QWidget>

class MusicListWidget : public QObject
{
    Q_OBJECT
public:
    explicit MusicListWidget(QObject *parent = nullptr);

signals:

};

#endif // MUSICLISTWIDGET_H
