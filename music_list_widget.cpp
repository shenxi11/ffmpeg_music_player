#include "music_list_widget.h"

MusicListWidget::MusicListWidget(QWidget *parent)
    :QListWidget(parent)
{
    setSpacing(10);

}
void MusicListWidget::addSong(const QString songName, const QString path)
{
    QSize size(width(), 30);
    MusicItem* itemWidget = new MusicItem(songName,path,"",size);

    QListWidgetItem *item = new QListWidgetItem(this);
    item->setData(Qt::UserRole, songName);

    pathMap[songName] = itemWidget;

    connect(itemWidget, &MusicItem::play_click, this, &MusicListWidget::_play_click);
    connect(itemWidget, &MusicItem::remove_click, this, &MusicListWidget::_remove_click);
    setItemWidget(item,itemWidget);
}
//void MusicListWidget::mouseDoubleClickEvent(QMouseEvent *event)
//{
//    QListWidgetItem* listItem = itemAt(event->pos());
//    MusicItem* item = dynamic_cast<MusicItem*>(itemWidget(listItem));

//    if(item)
//    {
//        emit selectMusic(item->getPath());
//        qDebug()<<"PATH:"<<item->getPath();
//    }

//    QListWidget::mouseDoubleClickEvent(event);
//}
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
void MusicListWidget::_play_click(QString songPath)
{
    emit play_click(songPath);
}
void MusicListWidget::_remove_click(QString songPath)
{
    QFileInfo fileInfo(songPath);
    QString songName = fileInfo.fileName();

    MusicItem* musicItem = pathMap[songName];


    for(int i = 0; i < this->count(); i++)
    {
        QListWidgetItem* item = this->item(i);

        if(item->data(Qt::UserRole).toString() == songName)
        {
            delete this->takeItem(i);
            pathMap.erase(songName);
            delete musicItem;
            emit remove_click(songName);
            break;
        }
    }
}
void MusicListWidget::Next_click(QString songName)
{
    if(this->count() <= 1)
    {
        return;
    }

    for(int i = 0; i < this->count(); i++)
    {
        QListWidgetItem* item = this->item(i);

        if(item->data(Qt::UserRole).toString() == songName)
        {
            if(i != this->count() - 1)
            {
                QListWidgetItem* item1 = this->item(i + 1);
                MusicItem* musicItem1 = pathMap[item1->data(Qt::UserRole).toString()];
                musicItem1->play_to_click();
            }
            else
            {
                QListWidgetItem* item1 = this->item(0);
                MusicItem* musicItem1 = pathMap[item1->data(Qt::UserRole).toString()];
                musicItem1->play_to_click();
            }
            break;
        }
    }
}
void MusicListWidget::Last_click(QString songName)
{
    if(this->count() <= 1)
    {
        return;
    }


    for(int i = 0; i < this->count(); i++)
    {
        QListWidgetItem* item = this->item(i);

        if(item->data(Qt::UserRole).toString() == songName)
        {
            if(i != 0)
            {
                QListWidgetItem* item1 = this->item(i - 1);
                MusicItem* musicItem1 = pathMap[item1->data(Qt::UserRole).toString()];
                musicItem1->play_to_click();
            }
            else
            {
                QListWidgetItem* item1 = this->item(this->count() - 1);
                MusicItem* musicItem1 = pathMap[item1->data(Qt::UserRole).toString()];
                musicItem1->play_to_click();
            }
            break;
        }
    }
}
