#include "music_item.h"

MusicItem::MusicItem(const QString& name, const QString& path, const QString& picPath, QSize size)
    :size(size)
{
    setAutoFillBackground(true);

    setFixedSize(size);

    music = std::make_shared<Music>();
    music->setPicPath(picPath);
    music->setSongPath(path);
    music->setPicPath(picPath);

    label = new QLabel(this);
    label->setText(name);
    label->move(size.height() * 2, 0);

    pic = new QLabel(this);
    pic->setFixedSize(size.height(), size.height());
    pic->move(0, 0);
    if(picPath=="")
    {
        pic->setStyleSheet(
                    "QLabel {"
                    "    border-image: url(:/new/prefix1/icon/Music.png);"
                    "}"
                    );
    }

    play = new QPushButton(this);
    play->setFixedSize(size.height(), size.height());
    play->move(size.width()/2, 0);
    play->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/play.png);"
                "}"
                );
    play->hide();
    connect(play, &QPushButton::clicked, this, &MusicItem::_play_click);

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
    emit signal_play_click(music->getSongPath());
}
void MusicItem::_remove_click()
{
    emit signal_remove_click(music->getSongPath());
}
void MusicItem::_play_click()
{
    emit signal_play_click(music->getSongPath());
}
void MusicItem::set_netflag(bool flag)
{
    this->isNetMusic = flag;
    if(this->isNetMusic)
    {
        if(!download)
        {
            download = new QPushButton(this);
            download->setFixedSize(size.height(), size.height());
            download->move(size.width()/2 + size.height(),0);
            download->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/下载.png);"
                        "}"
                        );
            download->hide();
            connect(download, &QPushButton::clicked, this, &MusicItem::on_signal_download_clicked);

        }
    }
    else
    {
        if(!remove)
        {
            remove = new QPushButton(this);
            remove->setFixedSize(size.height(), size.height());
            remove->move(size.width()/2 + size.height(),0);
            remove->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/delete.png);"
                        "}"
                        );
            remove->hide();
            connect(remove, &QPushButton::clicked, this, &MusicItem::_remove_click);
        }
    }
}
void MusicItem::on_signal_download_clicked()
{
    emit signal_download_click(music->getSongName());
}
