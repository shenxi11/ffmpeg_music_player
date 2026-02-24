#ifndef USER_H
#define USER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMutex>
#include <QMutexLocker>

class User : public QObject
{
    Q_OBJECT
public:
    static User* getInstance()
    {
        if (instance == nullptr)
        {
            QMutexLocker locker(&mutex);
            if (instance == nullptr)
            {
                instance = new User();
            }
        }
        return instance;
    }

    void set_username(QString username);
    void set_account(QString account);
    void set_password(QString password);
    void set_music_path(QStringList musics);

    QString get_username();
    QString get_account();
    QString get_password();
    QStringList get_music_path();

signals:
    void signal_add_songs();

private:
    User(QString account = "", QString password = "", QString username = "");
    User& operator=(const User a) = delete;
    User(const User&) = delete;

    static User* instance;
    static QMutex mutex;
    QString username;
    QString password;
    QString account;
    QStringList music_path;
};

#endif // USER_H
