#ifndef PIANWIDGET_QML_H
#define PIANWIDGET_QML_H

#include <QQuickWidget>
#include <QQuickItem>
#include <QWidget>
#include <QDebug>

class PianWidgetQml : public QQuickWidget
{
    Q_OBJECT
public:
    explicit PianWidgetQml(QWidget *parent = nullptr)
        : QQuickWidget(parent)
    {
        // 加载播放页顶部信息条 QML。
        setSource(QUrl("qrc:/qml/components/playback/PianWidget.qml"));
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        
        QQuickItem* root = rootObject();
        if (root) {
            // 透传“展开/收起”点击信号。
            connect(root, SIGNAL(upClicked()), this, SIGNAL(signalUpClick()));
        }
    }
    
    void setName(const QString& name) {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setName", Q_ARG(QVariant, name));
        }
    }
    
    void onSetPicPath(QString picPath) {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setPicPath", Q_ARG(QVariant, picPath));
        }
    }
    
signals:
    void signalUpClick();
};

#endif // PIANWIDGET_QML_H

