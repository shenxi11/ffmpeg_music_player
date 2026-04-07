#ifndef COVER_LOOKUP_H
#define COVER_LOOKUP_H

#include <QString>

QString normalizeMusicPathForLookup(QString path);
void rememberCoverForMusicPath(const QString& rawPath, const QString& rawCover);
QString queryCoverForMusicPath(const QString& rawPath);
void rememberCoverForSongMeta(const QString& title, const QString& artist, const QString& rawCover);
QString queryCoverForSongMeta(const QString& title, const QString& artist);
QString queryBestCoverForTrack(const QString& rawPath, const QString& title, const QString& artist);

#endif // COVER_LOOKUP_H
