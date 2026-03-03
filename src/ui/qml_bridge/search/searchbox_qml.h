#ifndef SEARCHBOX_QML_H
#define SEARCHBOX_QML_H

#include <QQuickWidget>
#include <QQmlContext>
#include <QQmlError>
#include <QQmlEngine>
#include <QQuickItem>
#include <QDebug>

/**
 * @brief QML 鐗堟湰鐨?SearchBox 鍖呰绫?
 * 鎻愪緵涓庡師 QWidget SearchBox 鐩稿悓鐨勬帴鍙ｏ紝鐢ㄤ簬鏃犵紳鏇挎崲
 */
class SearchBoxQml : public QQuickWidget
{
    Q_OBJECT

public:
    explicit SearchBoxQml(QWidget *parent = nullptr)
        : QQuickWidget(parent)
    {
        qDebug() << "SearchBoxQml: Initializing...";
        
        // 閰嶇疆 QML 寮曟搸鐨勫鍏ヨ矾寰?
        engine()->addImportPath("qrc:/");
        engine()->addImportPath(":/");
        
        // 娣诲姞 Qt 鐨?QML 妯″潡璺緞
        QStringList importPaths = engine()->importPathList();
        qDebug() << "SearchBoxQml: Current QML import paths:" << importPaths;
        
        // 璁剧疆 QML 寮曟搸灞炴€?
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        
        // 鍔犺浇 QML 鏂囦欢
        qDebug() << "SearchBoxQml: Loading QML from qrc:/qml/components/search/SearchBox.qml";
        setSource(QUrl("qrc:/qml/components/search/SearchBox.qml"));
        
        // 妫€鏌ュ姞杞芥槸鍚︽垚鍔?
        if (status() == QQuickWidget::Error) {
            qWarning() << "SearchBoxQml: Failed to load SearchBox.qml";
            QList<QQmlError> errors = this->errors();
            for (const QQmlError &error : errors) {
                qWarning() << "  Error:" << error.toString();
            }
            return;
        }
        
        qDebug() << "SearchBoxQml: QML loaded successfully, status =" << status();
        
        // 杩炴帴 QML 淇″彿鍒?C++ 淇″彿
        QQuickItem *rootItem = rootObject();
        if (rootItem) {
            qDebug() << "SearchBoxQml: Root item found, size:" << rootItem->width() << "x" << rootItem->height();
            
            // 杩炴帴 search(string) 淇″彿
            connect(rootItem, SIGNAL(search(QString)), 
                    this, SIGNAL(search(QString)));
            
            // 杩炴帴 searchAll() 淇″彿
            connect(rootItem, SIGNAL(searchAll()), 
                    this, SIGNAL(searchAll()));
            
            qDebug() << "SearchBoxQml: QML signals connected successfully";
        } else {
            qWarning() << "SearchBoxQml: Failed to get root object";
        }
        
        // 璁剧疆鑳屾櫙閫忔槑
        setClearColor(Qt::transparent);
        setAttribute(Qt::WA_AlwaysStackOnTop);
        setAttribute(Qt::WA_TranslucentBackground);
        
        qDebug() << "SearchBoxQml: Initialization complete";
    }
    
    ~SearchBoxQml() override = default;

signals:
    // 涓庡師 SearchBox 淇濇寔涓€鑷寸殑淇″彿
    void search(const QString &text);
    void searchAll();

public slots:
    /**
     * @brief 娓呯┖鎼滅储妗嗘枃鏈?
     */
    void clear() {
        QQuickItem *rootItem = rootObject();
        if (rootItem) {
            QMetaObject::invokeMethod(rootItem, "clear");
        }
    }
    
    /**
     * @brief 鑾峰彇褰撳墠鎼滅储鏂囨湰
     */
    QString text() const {
        QQuickItem *rootItem = const_cast<SearchBoxQml*>(this)->rootObject();
        if (rootItem) {
            return rootItem->property("text").toString();
        }
        return QString();
    }
    
    /**
     * @brief 璁剧疆鎼滅储鏂囨湰
     */
    void setText(const QString &text) {
        QQuickItem *rootItem = rootObject();
        if (rootItem) {
            rootItem->setProperty("text", text);
        }
    }
};

#endif // SEARCHBOX_QML_H

