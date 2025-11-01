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
        // 设置 QML 源文件
        setSource(QUrl("qrc:/qml/components/PianWidget.qml"));
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        
        QQuickItem* root = rootObject();
        if (root) {
            // 连接 QML 信号到 C++ 信号
            connect(root, SIGNAL(upClicked()), this, SIGNAL(signal_up_click()));
        }
    }
    
    void setName(const QString& name) {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setName", Q_ARG(QVariant, name));
        }
    }
    
    void on_signal_set_pic_path(QString picPath) {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setPicPath", Q_ARG(QVariant, picPath));
        }
    }
    
signals:
    void signal_up_click();
};

#endif // PIANWIDGET_QML_H
