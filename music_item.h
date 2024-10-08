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

    void button_op(bool flag);

    void _play_click();

    void _remove_click();

    void play_to_click();
signals:
    void play_click(QString songName);

    void remove_click(QString songName);
private:
    const QString name;

    const QString path;

    const QString picPath;

    const QSize size;

    QLabel* label;

    QLabel* pic;

    QPushButton* play;

    QPushButton* remove;

    bool hover = false;

protected:

    void enterEvent(QEvent *event) override
    {
        Q_UNUSED(event);
        hover = true;

        play->show();
        remove->show();

        update();
    }

    void leaveEvent(QEvent *event) override
    {
        Q_UNUSED(event);
        hover = false;

        play->hide();
        remove->hide();


        update();
    }


    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);

        QPainter painter(this);


        if (hover) {
            painter.setBrush(QColor(44, 210, 126));
        } else {
            painter.setBrush(QColor(246, 246, 246));
        }

        QPen pen;
        pen.setColor(Qt::black);
        pen.setWidth(2);
        painter.setPen(pen);

        painter.drawRect(this->rect());
    }

};

#endif // MUSICITEM_H
