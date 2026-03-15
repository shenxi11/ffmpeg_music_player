#include "user.h"

User* User::instance = nullptr;
QMutex User::mutex;

// User类构造函数实现
User::User(QString account, QString password, QString username)
    : account(account), password(password), username(username)
{
}

// User类setter实现
void User::setUsername(QString username)
{
    this->username = username;
}

void User::setAccount(QString account)
{
    this->account = account;
}

void User::setPassword(QString password)
{
    this->password = password;
}

void User::setMusicPath(QStringList musics)
{
    this->music_path = musics;
    emit signalAddSongs();
}

// User类getter实现
QString User::getUsername()
{
    return username;
}

QString User::getAccount()
{
    return account;
}

QString User::getPassword()
{
    return password;
}

QStringList User::getMusicPath()
{
    return this->music_path;
}
