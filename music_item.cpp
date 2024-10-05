#include "music_item.h"

MusicItem::MusicItem(const QString& name, const QString& path, const QString& picPath, QSize size)
    : name(name)
    , path(path)
    , picPath(picPath)
{
    setFixedSize(size);

    label = new QLabel(this);

    pic = new QPushButton(this);

    play = new QPushButton(this);
    remove = new QPushButton(this);

    label->setText(name);

    pic->setFixedSize(size.height(),size.height());
    play->setFixedSize(size.height(),size.height());
    remove->setFixedSize(size.height(),size.height());

    label->move(size.height(),0);

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
                "    border-image: url(:/new/prefix1/icon/upload.png);"
                "}"
                );
    if(picPath=="")
    {
        pic->setStyleSheet(
                    "QPushButton {"
                    "    border-image: url(:/new/prefix1/icon/Music.png);"
                    "}"
                    );
    }

}
QString MusicItem::getName()
{
    return this->name;
}
QString MusicItem::getPath()
{
    return this->path;
}
