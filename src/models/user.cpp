#include "user.h"

User* User::instance = nullptr;
QMutex User::mutex;

// User类构造函数实现
User::User(QString account, QString password, QString username)
    : account(account), password(password), username(username)
{
}

// User类setter实现
void User::set_username(QString username)
{
    this->username = username;
}

void User::set_account(QString account)
{
    this->account = account;
}

void User::set_password(QString password)
{
    this->password = password;
}

void User::set_music_path(QStringList musics)
{
    this->music_path = musics;
    emit signal_add_songs();
}

// User类getter实现
QString User::get_username()
{
    return username;
}

QString User::get_account()
{
    return account;
}

QString User::get_password()
{
    return password;
}

QStringList User::get_music_path()
{
    return this->music_path;
}
