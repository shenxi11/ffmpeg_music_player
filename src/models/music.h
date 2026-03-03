#ifndef MUSIC_H
#define MUSIC_H

#include <QObject>
#include <QFileInfo>
#include <QMetaType>

#include "headers.h"

class Music{
public:
    Music();
    Music(const Music& music);

    void setSongPath(QString songPath);
    void setMusicID(QString musicID);
    void setPicPath(QString picPath);
    void setLrcPath(QString lrcPath);
    void setSinger(QString singer);
    void setDuration(long duration);

    QString getSongPath() const;
    QString getSongName() const;
    QString getMusicID() const;
    QString getSinger() const;
    long getDuration() const;
    QString getLrcPath() const;
    QString getPicPath() const;

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
