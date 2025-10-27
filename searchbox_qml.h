#ifndef SEARCHBOX_QML_H
#define SEARCHBOX_QML_H

#include <QQuickWidget>
#include <QQmlContext>
#include <QQmlError>
#include <QQmlEngine>
#include <QQuickItem>
#include <QDebug>

/**
 * @brief QML 版本的 SearchBox 包装类
 * 提供与原 QWidget SearchBox 相同的接口，用于无缝替换
 */
class SearchBoxQml : public QQuickWidget
{
    Q_OBJECT

public:
    explicit SearchBoxQml(QWidget *parent = nullptr)
        : QQuickWidget(parent)
    {
        qDebug() << "SearchBoxQml: Initializing...";
        
        // 配置 QML 引擎的导入路径
        engine()->addImportPath("qrc:/");
        engine()->addImportPath(":/");
        
        // 添加 Qt 的 QML 模块路径
        QStringList importPaths = engine()->importPathList();
        qDebug() << "SearchBoxQml: Current QML import paths:" << importPaths;
        
        // 设置 QML 引擎属性
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        
        // 加载 QML 文件
        qDebug() << "SearchBoxQml: Loading QML from qrc:/qml/components/SearchBox.qml";
        setSource(QUrl("qrc:/qml/components/SearchBox.qml"));
        
        // 检查加载是否成功
        if (status() == QQuickWidget::Error) {
            qWarning() << "SearchBoxQml: Failed to load SearchBox.qml";
            QList<QQmlError> errors = this->errors();
            for (const QQmlError &error : errors) {
                qWarning() << "  Error:" << error.toString();
            }
            return;
        }
        
        qDebug() << "SearchBoxQml: QML loaded successfully, status =" << status();
        
        // 连接 QML 信号到 C++ 信号
        QQuickItem *rootItem = rootObject();
        if (rootItem) {
            qDebug() << "SearchBoxQml: Root item found, size:" << rootItem->width() << "x" << rootItem->height();
            
            // 连接 search(string) 信号
            connect(rootItem, SIGNAL(search(QString)), 
                    this, SIGNAL(search(QString)));
            
            // 连接 searchAll() 信号
            connect(rootItem, SIGNAL(searchAll()), 
                    this, SIGNAL(searchAll()));
            
            qDebug() << "SearchBoxQml: QML signals connected successfully";
        } else {
            qWarning() << "SearchBoxQml: Failed to get root object";
        }
        
        // 设置背景透明
        setClearColor(Qt::transparent);
        setAttribute(Qt::WA_AlwaysStackOnTop);
        setAttribute(Qt::WA_TranslucentBackground);
        
        qDebug() << "SearchBoxQml: Initialization complete";
    }
    
    ~SearchBoxQml() override = default;

signals:
    // 与原 SearchBox 保持一致的信号
    void search(const QString &text);
    void searchAll();

public slots:
    /**
     * @brief 清空搜索框文本
     */
    void clear() {
        QQuickItem *rootItem = rootObject();
        if (rootItem) {
            QMetaObject::invokeMethod(rootItem, "clear");
        }
    }
    
    /**
     * @brief 获取当前搜索文本
     */
    QString text() const {
        QQuickItem *rootItem = const_cast<SearchBoxQml*>(this)->rootObject();
        if (rootItem) {
            return rootItem->property("text").toString();
        }
        return QString();
    }
    
    /**
     * @brief 设置搜索文本
     */
    void setText(const QString &text) {
        QQuickItem *rootItem = rootObject();
        if (rootItem) {
            rootItem->setProperty("text", text);
        }
    }
};

#endif // SEARCHBOX_QML_H
