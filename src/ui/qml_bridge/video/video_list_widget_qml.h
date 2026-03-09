#ifndef VIDEO_LIST_WIDGET_QML_H
#define VIDEO_LIST_WIDGET_QML_H

#include <QQuickWidget>
#include <QQuickItem>
#include <QWidget>
#include <QVariant>
#include <QVariantMap>
#include <QDebug>

/**
 * @brief 在线视频列表QML控件包装类
 * 类似于MusicListWidgetNetQml，用于显示在线视频列表
 */
class VideoListWidgetQml : public QQuickWidget
{
    Q_OBJECT
public:
    explicit VideoListWidgetQml(QWidget *parent = nullptr);

    /**
     * @brief 添加视频到列表
     * @param name 视频名称
     * @param path 视频路径
     * @param size 文件大小（字节）
     */
    void addVideo(const QString& name, const QString& path, qint64 size);

    /**
     * @brief 批量添加视频列表
     * @param videoList 视频列表（QVariantList，每个元素包含name、path、size）
     */
    void addVideoList(const QVariantList& videoList);

    /**
     * @brief 清空视频列表
     */
    void clearAll();

    /**
     * @brief 获取视频数量
     * @return 视频数量
     */
    int getCount();

signals:
    /**
     * @brief 视频被选中信号
     * @param videoPath 视频路径（相对路径，如 an_hao.mp4）
     * @param videoName 视频名称
     */
    void signal_video_selected(QString videoPath, QString videoName);
    
    /**
     * @brief 刷新请求信号
     */
    void signal_refresh_requested();
};

#endif // VIDEO_LIST_WIDGET_QML_H
