#include "music_list_widget.h"

MusicListWidget::MusicListWidget(QWidget *parent)
    :QListWidget(parent)
{
    setSpacing(5);
}
void MusicListWidget::onAddSong(const QString songName, const QString path, bool isNetMusic)
{
    if(pathMap[songName])
    {
        return;
    }
    QSize size(width(), 60);  // 增加高度以适应 QML 版本
    // 使用 QML 版本的 MusicItem
    MusicItemQml* itemWidget = new MusicItemQml(songName, path, "", size);
    // itemWidget->setNetFlag(isNetMusic);  // QML 版本通过 setProperty 设置

    QListWidgetItem *item = new QListWidgetItem(this);
    item->setData(Qt::UserRole, songName);

    pathMap[songName] = itemWidget;

    connect(itemWidget, &MusicItemQml::signalPlayClick, this, &MusicListWidget::playClick);
    connect(itemWidget, &MusicItemQml::signalRemoveClick, this, &MusicListWidget::removeClick);
    connect(itemWidget, &MusicItemQml::signalDownloadClick, this, &MusicListWidget::signalDownloadClick);

    setItemWidget(item,itemWidget);
}

void MusicListWidget::receiveSongOp(bool flag, QString fileName)
{
    if(pathMap[fileName])
    {
        // 尝试转换为 MusicItemQml
        if (auto qmlItem = qobject_cast<MusicItemQml*>(pathMap[fileName])) {
            qmlItem->buttonOp(flag);
        }
        // 兼容旧版 MusicItem
        else if (auto oldItem = qobject_cast<MusicItem*>(pathMap[fileName])) {
            oldItem->buttonOp(flag);
        }
    }

    if(flag)
    {
        for(auto &i: pathMap)
        {
            if(i.first != fileName)
            {
                if (auto qmlItem = qobject_cast<MusicItemQml*>(i.second)) {
                    qmlItem->buttonOp(false);
                }
                else if (auto oldItem = qobject_cast<MusicItem*>(i.second)) {
                    oldItem->buttonOp(false);
                }
            }
        }
    }
}
void MusicListWidget::playClick(QString songPath)
{
    emit play_click(songPath);
}
void MusicListWidget::removeClick(QString songPath)
{
    QFileInfo fileInfo(songPath);
    QString songName = fileInfo.fileName();

    QWidget* musicItem = pathMap[songName];


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
void MusicListWidget::nextClick(QString songName)
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
                QWidget* widget = pathMap[item1->data(Qt::UserRole).toString()];
                if (auto qmlItem = qobject_cast<MusicItemQml*>(widget)) {
                    qmlItem->playToClick();
                } else if (auto oldItem = qobject_cast<MusicItem*>(widget)) {
                    oldItem->playToClick();
                }
            }
            else
            {
                QListWidgetItem* item1 = this->item(0);
                QWidget* widget = pathMap[item1->data(Qt::UserRole).toString()];
                if (auto qmlItem = qobject_cast<MusicItemQml*>(widget)) {
                    qmlItem->playToClick();
                } else if (auto oldItem = qobject_cast<MusicItem*>(widget)) {
                    oldItem->playToClick();
                }
            }
            break;
        }
    }
}
void MusicListWidget::lastClick(QString songName)
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
                QWidget* widget = pathMap[item1->data(Qt::UserRole).toString()];
                if (auto qmlItem = qobject_cast<MusicItemQml*>(widget)) {
                    qmlItem->playToClick();
                } else if (auto oldItem = qobject_cast<MusicItem*>(widget)) {
                    oldItem->playToClick();
                }
            }
            else
            {
                QListWidgetItem* item1 = this->item(this->count() - 1);
                QWidget* widget = pathMap[item1->data(Qt::UserRole).toString()];
                if (auto qmlItem = qobject_cast<MusicItemQml*>(widget)) {
                    qmlItem->playToClick();
                } else if (auto oldItem = qobject_cast<MusicItem*>(widget)) {
                    oldItem->playToClick();
                }
            }
            break;
        }
    }
}
void MusicListWidget::onAddSonglist(const QStringList filename_list, const QList<double> duration)
{
    clear();
    for(auto i: filename_list)
    {
        onAddSong(i, i, true);
    }
}
void MusicListWidget::removeAll()
{
    clear();
}
