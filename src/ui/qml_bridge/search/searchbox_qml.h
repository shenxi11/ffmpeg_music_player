#ifndef SEARCHBOX_QML_H
#define SEARCHBOX_QML_H

#include <QQuickWidget>
#include <QQmlContext>
#include <QQmlError>
#include <QQmlEngine>
#include <QQuickItem>
#include <QDebug>

/**
 * QML 搜索框桥接控件，负责加载 SearchBox.qml 并转发搜索信号。
 * 用于在 QWidget 页面中复用 QML 搜索输入体验。
 */
class SearchBoxQml : public QQuickWidget
{
    Q_OBJECT

public:
    explicit SearchBoxQml(QWidget *parent = nullptr)
        : QQuickWidget(parent)
    {
        qDebug() << "SearchBoxQml: Initializing...";
        
        // 补充 qrc 导入路径，避免嵌套组件解析失败。
        engine()->addImportPath("qrc:/");
        engine()->addImportPath(":/");
        
        // 输出导入路径，便于排查 QML 资源加载问题。
        QStringList importPaths = engine()->importPathList();
        qDebug() << "SearchBoxQml: Current QML import paths:" << importPaths;
        
        // 根节点尺寸跟随控件尺寸变化。
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        
        // 加载搜索框 QML 资源。
        qDebug() << "SearchBoxQml: Loading QML from qrc:/qml/components/search/SearchBox.qml";
        setSource(QUrl("qrc:/qml/components/search/SearchBox.qml"));
        
        // 启动阶段遇到错误时输出完整错误栈。
        if (status() == QQuickWidget::Error) {
            qWarning() << "SearchBoxQml: Failed to load SearchBox.qml";
            QList<QQmlError> errors = this->errors();
            for (const QQmlError &error : errors) {
                qWarning() << "  Error:" << error.toString();
            }
            return;
        }
        
        qDebug() << "SearchBoxQml: QML loaded successfully, status =" << status();
        
        // 获取根对象并建立搜索信号转发。
        QQuickItem *rootItem = rootObject();
        if (rootItem) {
            qDebug() << "SearchBoxQml: Root item found, size:" << rootItem->width() << "x" << rootItem->height();
            
            // 转发关键字搜索信号。
            connect(rootItem, SIGNAL(search(QString)), 
                    this, SIGNAL(search(QString)));
            
            // 转发“全局搜索”信号。
            connect(rootItem, SIGNAL(searchAll()), 
                    this, SIGNAL(searchAll()));
            
            qDebug() << "SearchBoxQml: QML signals connected successfully";
        } else {
            qWarning() << "SearchBoxQml: Failed to get root object";
        }
        
        // 使用透明背景，避免遮挡父级主题背景。
        setClearColor(Qt::transparent);
        setAttribute(Qt::WA_AlwaysStackOnTop);
        setAttribute(Qt::WA_TranslucentBackground);
        
        qDebug() << "SearchBoxQml: Initialization complete";
    }
    
    ~SearchBoxQml() override = default;

signals:
    // 与原搜索框保持一致的接口
    void search(const QString &text);
    void searchAll();

public slots:
    /**
     * 清空搜索输入框内容。
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
     * 外部设置搜索框文本内容。
     */
    void setText(const QString &text) {
        QQuickItem *rootItem = rootObject();
        if (rootItem) {
            rootItem->setProperty("text", text);
        }
    }
};

#endif // SEARCHBOX_QML_H

