#ifndef MUSICITEM_H
#define MUSICITEM_H

#include <QObject>
#include <QListWidgetItem>
#include "headers.h"
#include "music.h"
class MusicItem :  public QWidget
{
    Q_OBJECT
public:
    explicit MusicItem(const QString& name, const QString& path, const QString& picPath, QSize size);
    ~MusicItem(){};

    std::shared_ptr<Music> getMusic(){return this->music;};
    void button_op(bool flag);
    void _play_click();
    void _remove_click();
    void play_to_click();
    void on_signal_download_clicked();
    void set_netflag(bool flag);
signals:
    void signal_play_click(QString songName);
    void signal_remove_click(QString songName);
    void signal_download_click(QString songName);
private:
    const QSize size;
    QLabel* label;
    QLabel* pic;
    QPushButton* play= nullptr;
    QPushButton* remove = nullptr;
    QPushButton* download = nullptr;

    std::shared_ptr<Music> music;

    bool hover = false;
    bool isNetMusic = false;

protected:

    void enterEvent(QEvent *event) override
    {
        Q_UNUSED(event);
        hover = true;

        play->show();
        if(remove)
            remove->show();
        if(download)
            download->show();

        update();
    }

    void leaveEvent(QEvent *event) override
    {
        Q_UNUSED(event);
        hover = false;

        play->hide();
        if(remove)
            remove->hide();
        if(download)
            download->hide();

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
