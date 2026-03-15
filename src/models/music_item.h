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
    ~MusicItem();

    std::shared_ptr<Music> getMusic();
    void buttonOp(bool flag);
    void playClick();
    void removeClick();
    void playToClick();
    void onDownloadClicked();
    void setNetFlag(bool flag);
signals:
    void signalPlayClick(QString songName);
    void signalRemoveClick(QString songName);
    void signalDownloadClick(QString songName);
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
