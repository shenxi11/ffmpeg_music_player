#include "music_item.h"

MusicItem::MusicItem(const QString& name, const QString& path, const QString& picPath, QSize size)
    : name(name)
    , path(path)
    , picPath(picPath)
{
    setAutoFillBackground(true);

    setFixedSize(size);

    label = new QLabel(this);

    pic = new QLabel(this);

    play = new QPushButton(this);
    remove = new QPushButton(this);

    label->setText(name);

    pic->setFixedSize(size.height(),size.height());
    play->setFixedSize(size.height(),size.height());
    remove->setFixedSize(size.height(),size.height());

    label->move(size.height()*2,0);

    pic->move(0,0);
    play->move(size.width()/2,0);
    remove->move(size.width()/2+size.height(),0);


    play->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/play.png);"
                "}"
                );
    remove->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/delete.png);"
                "}"
                );
    if(picPath=="")
    {
        pic->setStyleSheet(
                    "QLabel {"
                    "    border-image: url(:/new/prefix1/icon/Music.png);"
                    "}"
                    );
    }

    play->hide();
    remove->hide();

    connect(play, &QPushButton::clicked, this, &MusicItem::_play_click);
    connect(remove, &QPushButton::clicked, this, &MusicItem::_remove_click);
}
void MusicItem::button_op(bool flag)
{
    if(flag)
    {
        play->setStyleSheet(
                    "QPushButton {"
                    "    border-image: url(:/new/prefix1/icon/pause.png);"
                    "}"
                    );
    }
    else
    {
        play->setStyleSheet(
                    "QPushButton {"
                    "    border-image: url(:/new/prefix1/icon/play.png);"
                    "}"
                    );
    }
}
void MusicItem::play_to_click()
{
    emit play_click(this->path);
}
void MusicItem::_remove_click()
{
    emit remove_click(this->path);
}
void MusicItem::_play_click()
{
    emit play_click(this->path);
}
QString MusicItem::getName()
{
    return this->name;
}
QString MusicItem::getPath()
{
    return this->path;
}
