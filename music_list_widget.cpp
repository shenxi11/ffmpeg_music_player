#include "music_list_widget.h"

MusicListWidget::MusicListWidget(QWidget *parent)
    :QListWidget(parent)
{

}
void MusicListWidget::addSong(const QString songName, const QString path)
{
    addItem(songName);
    pathMap[songName] = path;
}
void MusicListWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    QListWidgetItem *item = itemAt(event->pos());

    if(item)
    {
        emit selectMusic(pathMap[item->text()]);
    }

    QListWidget::mouseDoubleClickEvent(event);
}
