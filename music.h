#ifndef MUSIC_H
#define MUSIC_H

#include <QObject>
#include <QFileInfo>
#include <QMetaType>

#include "headers.h"

class Music{
public:
    Music(){};
    Music(const Music& music){
        setSongPath(music.songPath);
        setSinger(music.singer);
        setLrcPath(music.lrcPath);
        setMusicID(music.musicID);
        setDuration(music.duration);
        setPicPath(music.picPath);
    };

    void setSongPath(QString songPath){this->songPath = songPath;};
    void setMusicID(QString musicID){this->musicID = musicID;};
    void setPicPath(QString picPath){this->picPath = picPath;};
    void setLrcPath(QString lrcPath){this->lrcPath = lrcPath;};
    void setSinger(QString singer){this->singer = singer;};
    void setDuration(long duration){this->duration = duration;};

    QString getSongPath() const {return this->songPath;};
    QString getSongName() const
    {
        QFileInfo file(this->songPath);
        return file.fileName();
    };
    QString getMusicID() const {return this->musicID;};
    QString getSinger() const {return this->singer;};
    long getDuration() const {return this->duration;};
    QString getLrcPath() const {return this->lrcPath;};
    QString getPicPath() const {return this->picPath;};

private:
    QString musicID;
    QString songName;
    QString songPath;
    QString singer;
    long duration = 0;
    QString picPath;
    QString lrcPath;
};

Q_DECLARE_METATYPE(Music)

#endif // MUSIC_H
