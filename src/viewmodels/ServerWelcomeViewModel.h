#ifndef SERVERWELCOMEVIEWMODEL_H
#define SERVERWELCOMEVIEWMODEL_H

#include "BaseViewModel.h"

#include <QJsonObject>
#include <QPoint>

class QNetworkAccessManager;

/**
 * @brief 欢迎页视图模型。
 *
 * 负责服务端可达性校验与服务地址配置持久化，
 * 欢迎页对话框只负责窗口展示与交互。
 */
class ServerWelcomeViewModel : public BaseViewModel
{
    Q_OBJECT

public:
    explicit ServerWelcomeViewModel(QObject* parent = nullptr);

    QString serverHost() const;
    int serverPort() const;
    bool hasWindowPos() const;
    QPoint windowPos() const;
    void setWindowPos(const QPoint& point);

    bool verifyServer(const QString& rawHost,
                      int currentPort,
                      QString* normalizedHost,
                      int* normalizedPort,
                      QString* errorMessage);

private:
    static QString normalizeHostInput(const QString& rawHost, int* portInOut);
    bool getJson(const QString& fullUrl, QJsonObject* root, QString* errorMessage);

    QNetworkAccessManager* m_networkManager = nullptr;
};

#endif // SERVERWELCOMEVIEWMODEL_H
