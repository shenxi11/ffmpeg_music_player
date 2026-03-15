#ifndef MUSIC_LIST_WIDGET_NET_QML_H
#define MUSIC_LIST_WIDGET_NET_QML_H

#include <QQuickWidget>
#include <QQmlContext>
#include <QWidget>
#include <QQuickItem>
#include <QDebug>

/**
 * @brief 网络音乐列表组件 ViewModel (MVVM模式)
 * 
 * MVVM架构：
 * - Q_PROPERTY 暴露属性供 QML 绑定
 * - QML 作为纯 View 层，只负责显示
 * - 业务逻辑在 C++ 中处理
 */
class MusicListWidgetNetQml : public QQuickWidget
{
    Q_OBJECT
    
    // ===== MVVM 属性绑定 =====
    Q_PROPERTY(QString currentPlayingPath READ currentPlayingPath WRITE setCurrentPlayingPath NOTIFY currentPlayingPathChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying WRITE setIsPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(int songCount READ songCount NOTIFY songCountChanged)
    
public:
    explicit MusicListWidgetNetQml(QWidget *parent = nullptr);
    
    // ===== MVVM 属性访问器 =====
    QString currentPlayingPath() const { return m_currentPlayingPath; }
    bool isPlaying() const { return m_isPlaying; }
    int songCount() const { return m_songCount; }
    
    // ===== 属性设置器（带数据绑定通知）=====
    void setCurrentPlayingPath(const QString& path);
    void setIsPlaying(bool playing);
    void setPlayingState(const QString& filePath, bool playing);

    // ===== 业务逻辑方法 =====
    void addSong(const QString& songName, const QString& filePath, 
                 const QString& artist = "", const QString& duration = "0:00",
                 const QString& cover = "");
    void addSongList(const QStringList& songNames, const QStringList& relativePaths, 
                     const QList<double>& durations, const QStringList& coverUrls = QStringList(), 
                     const QStringList& artists = QStringList());
    void removeSong(const QString& filePath);
    void clearAll();
    int getCount();
    void playNext(const QString& songName);
    void playLast(const QString& songName);
    
signals:
    // ===== MVVM 属性变化信号 =====
    void currentPlayingPathChanged();
    void isPlayingChanged();
    void songCountChanged();
    
    // ===== 业务逻辑信号 =====
    void signalPlayClick(QString path, QString artist, QString cover);
    void signalRemoveClick(QString path);
    void signalDownloadClick(QString path);
    void signalNext(QString songName);
    void signalLast(QString songName);
    void addToFavorite(QString path, QString title, QString artist, QString duration);

private:
    // ===== MVVM 数据模型 =====
    QString m_currentPlayingPath;
    bool m_isPlaying;
    int m_songCount;
    
    void updateSongCount();
};

#endif // MUSIC_LIST_WIDGET_NET_QML_H
