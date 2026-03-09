#ifndef MUSICLISTWIDGETLOCAL_H
#define MUSICLISTWIDGETLOCAL_H

#include <QObject>
#include <QWidget>
#include "music_list_widget_qml.h"
#include "httprequest_v2.h"

class MusicListWidgetLocal : public QWidget
{
    Q_OBJECT
public:
    explicit MusicListWidgetLocal(QWidget *parent = nullptr);
    
    // 获取内部的QML列表控件
    MusicListWidgetQml* getListWidget() const { return listWidget; }
    
public slots:
    void on_signal_add_button_clicked();
    void on_signal_add_song(const QString filename, const QString path);
    void on_signal_play_button_click(bool flag, const QString filename);
    void on_signal_play_click(const QString songName);
    void on_signal_remove_click(const QString songeName);
    void on_signal_translate_button_clicked();
    void on_signal_update_metadata(QString filePath, QString coverUrl, QString duration);
signals:
    void signal_add_button_clicked();
    void signal_add_song(const QString filename, const QString path);
    void signal_play_button_click(bool flag, const QString filename);
    void signal_play_click(const QString songName, bool flag);
    void signal_remove_click(const QString songName);
    void signal_last(QString songName);
    void signal_next(QString songName);
    void signal_translate_button_clicked();
private:
    MusicListWidgetQml* listWidget;
    QPushButton* add;
    HttpRequestV2* request;
};

#endif // MUSICLISTWIDGETLOCAL_H
