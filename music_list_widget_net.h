#ifndef MUSICLISTWIDGETNET_H
#define MUSICLISTWIDGETNET_H

#include <QObject>
#include <QWidget>
#include <QStringList>
#include <QMap>
#include "music_list_widget.h"
#include "translate_widget.h"
#include "httprequest.h"
class MusicListWidgetNet : public QWidget
{
    Q_OBJECT
public:
    explicit MusicListWidgetNet(QWidget *parent = nullptr);
    void on_signal_play_click(const QString name);
    void on_signal_remove_click(const QString name);
    void on_signal_play_button_click(bool flag, const QString filename);
    void on_signal_set_down_dir(QString down_dir);
    void on_signal_download_music(QString songName);
    void on_signal_translate_button_clicked();
signals:
    void signal_add_songlist(const QStringList filename_list, const QList<double> duration);
    void signal_play_click(const QString songName, bool net);
    void signal_play_button_click(bool flag, const QString filename);
    void signal_last(QString songName);
    void signal_next(QString songName);
    void signal_choose_download_dir();
    void signal_translate_button_clicked();

private:
    MusicListWidget* listWidget;

    QMap<QString, double> song_duration;
    QPushButton* download_dir;
    QPushButton* translateBtn;
    QLabel* dir_label;

    QString down_dir;
    HttpRequest* request;
};

#endif // MUSICLISTWIDGETNET_H
