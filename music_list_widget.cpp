#include "music_list_widget.h"

MusicListWidget::MusicListWidget(QWidget *parent)
    :QListWidget(parent)
{
    setSpacing(10);

}
void MusicListWidget::addSong(const QString songName, const QString path)
{
    QSize size(width(), 50);
    MusicItem* itemWidget = new MusicItem(songName,path,"",size);

    QListWidgetItem *item = new QListWidgetItem(this);
    item->setData(Qt::UserRole, songName);

    pathMap[songName] = itemWidget;

    connect(itemWidget, &MusicItem::play_click, this, &MusicListWidget::_play_click);
    connect(itemWidget, &MusicItem::remove_click, this, &MusicListWidget::_remove_click);
    setItemWidget(item,itemWidget);
}
void MusicListWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    MusicItem* item = dynamic_cast<MusicItem*>(itemAt(event->pos()));

    if(item)
    {
        emit selectMusic(item->getPath());
    }

    QListWidget::mouseDoubleClickEvent(event);
}
void MusicListWidget::receive_song_op(bool flag, QString fileName)
{
    pathMap[fileName]->button_op(flag);

    if(flag)
    {
        for(auto &i: pathMap)
        {
            if(i.first != fileName)
            {
                i.second->button_op(false);
            }
        }
    }
}
void MusicListWidget::_play_click(QString songName)
{
    emit play_click(songName);
}
void MusicListWidget::_remove_click(QString songName)
{
    MusicItem* musicItem = pathMap[songName];


    for(int i = 0; i < this->count(); i++)
    {
        QListWidgetItem* item = this->item(i);

        if(item->data(Qt::UserRole).toString() == songName)
        {
            delete this->takeItem(i);
            pathMap.erase(songName);
            delete musicItem;
            break;
        }
    }
}
