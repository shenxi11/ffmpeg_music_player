#ifndef MUSIC_H
#define MUSIC_H

#include <QObject>
#include <QFileInfo>

#include "headers.h"

class Music{
public:
    Music(){};
    Music(Music& music){
        setSongPath(music.getSongPath());
        setSinger(music.getSinger());
        setLrcPath(music.getLrcPath());
        setMusicID(music.getMusicID());
        setDuration(music.getDuration());
    };

    void setSongPath(QString songPath){this->songPath = songPath;};
    void setMusicID(QString musicID){this->musicID = musicID;};
    void setPicPath(QString picPath){this->picPath = picPath;};
    void setLrcPath(QString lrcPath){this->lrcPath = lrcPath;};
    void setSinger(QString singer){this->singer = singer;};
    void setDuration(long duration){this->duration = duration;};

    QString getSongPath(){return this->songPath;};
    QString getSongName()
    {
        auto file = new QFileInfo(this->songPath);
        this->songName = file->fileName();
        return this->songName;
    };
    QString getMusicID(){return this->musicID;};
    QString getSinger(){return this->singer;};
    long getDuration(){return this->duration;};
    QString getLrcPath(){return this->lrcPath;};
    QString getPicPath(){return this->picPath;};

private:
    QString musicID;
    QString songName;
    QString songPath;
    QString singer;
    long duration;
    QString picPath;
    QString lrcPath;
};

#endif // MUSIC_H
