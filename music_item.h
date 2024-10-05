#ifndef MUSICITEM_H
#define MUSICITEM_H

#include <QObject>
#include <QListWidgetItem>
#include "headers.h"

class MusicItem :  public QWidget
{
    Q_OBJECT
public:
    explicit MusicItem(const QString& name, const QString& path, const QString& picPath, QSize size);

    QString getName();

    QString getPath();
signals:

private:
    const QString name;

    const QString path;

    const QString picPath;

    const QSize size;

    QLabel* label;

    QPushButton* pic;

    QPushButton* play;

    QPushButton* remove;



};

#endif // MUSICITEM_H
