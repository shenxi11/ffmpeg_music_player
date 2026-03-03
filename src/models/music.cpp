#include "music.h"

Music::Music()
{
}

Music::Music(const Music& music)
{
    setSongPath(music.songPath);
    setSinger(music.singer);
    setLrcPath(music.lrcPath);
    setMusicID(music.musicID);
    setDuration(music.duration);
    setPicPath(music.picPath);
}

void Music::setSongPath(QString songPath)
{
    this->songPath = songPath;
}

void Music::setMusicID(QString musicID)
{
    this->musicID = musicID;
}

void Music::setPicPath(QString picPath)
{
    this->picPath = picPath;
}

void Music::setLrcPath(QString lrcPath)
{
    this->lrcPath = lrcPath;
}

void Music::setSinger(QString singer)
{
    this->singer = singer;
}

void Music::setDuration(long duration)
{
    this->duration = duration;
}

QString Music::getSongPath() const
{
    return this->songPath;
}

QString Music::getSongName() const
{
    QFileInfo file(this->songPath);
    return file.fileName();
}

QString Music::getMusicID() const
{
    return this->musicID;
}

QString Music::getSinger() const
{
    return this->singer;
}

long Music::getDuration() const
{
    return this->duration;
}

QString Music::getLrcPath() const
{
    return this->lrcPath;
}

QString Music::getPicPath() const
{
    return this->picPath;
}
