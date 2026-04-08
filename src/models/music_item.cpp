#include "music_item.h"

MusicItem::MusicItem(const QString& name, const QString& path, const QString& picPath, QSize size)
    : size(size) {
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
    if (picPath == "") {
        pic->setStyleSheet("QLabel {"
                           "    border-image: url(:/qml/assets/ai/icons/default-music-cover.svg);"
                           "}");
    }

    play = new QPushButton(this);
    play->setFixedSize(size.height(), size.height());
    play->move(size.width() / 2, 0);
    play->setStyleSheet("QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/play.png);"
                        "}");
    play->hide();
    connect(play, &QPushButton::clicked, this, &MusicItem::playClick);
}
void MusicItem::buttonOp(bool flag) {
    if (flag) {
        play->setStyleSheet("QPushButton {"
                            "    border-image: url(:/new/prefix1/icon/pause.png);"
                            "}");

    } else {
        play->setStyleSheet("QPushButton {"
                            "    border-image: url(:/new/prefix1/icon/play.png);"
                            "}");
    }
}
void MusicItem::playToClick() {
    emit signalPlayClick(music->getSongPath());
}
void MusicItem::removeClick() {
    emit signalRemoveClick(music->getSongPath());
}
void MusicItem::playClick() {
    emit signalPlayClick(music->getSongPath());
}
void MusicItem::setNetFlag(bool flag) {
    this->isNetMusic = flag;
    if (this->isNetMusic) {
        if (!download) {
            download = new QPushButton(this);
            download->setFixedSize(size.height(), size.height());
            download->move(size.width() / 2 + size.height(), 0);
            download->setStyleSheet("QPushButton {"
                                    "    border-image: url(:/new/prefix1/icon/download.png);"
                                    "}");
            download->hide();
            connect(download, &QPushButton::clicked, this, &MusicItem::onDownloadClicked);
        }
    } else {
        if (!remove) {
            remove = new QPushButton(this);
            remove->setFixedSize(size.height(), size.height());
            remove->move(size.width() / 2 + size.height(), 0);
            remove->setStyleSheet("QPushButton {"
                                  "    border-image: url(:/new/prefix1/icon/delete.png);"
                                  "}");
            remove->hide();
            connect(remove, &QPushButton::clicked, this, &MusicItem::removeClick);
        }
    }
}

MusicItem::~MusicItem() {}

std::shared_ptr<Music> MusicItem::getMusic() {
    return this->music;
}

void MusicItem::onDownloadClicked() {
    emit signalDownloadClick(music->getSongName());
}
