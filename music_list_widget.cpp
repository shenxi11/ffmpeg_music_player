#include "music_list_widget.h"

MusicListWidget::MusicListWidget(QWidget *parent)
    :QListWidget(parent)
{
    setSpacing(10);

}
void MusicListWidget::addSong(const QString songName, const QString path)
{
    QSize size(width(),30);
    MusicItem* itemWidget = new MusicItem(songName,path,"",size);

    QListWidgetItem *item = new QListWidgetItem(this);

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
