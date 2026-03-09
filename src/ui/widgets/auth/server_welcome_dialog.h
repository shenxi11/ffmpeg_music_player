#ifndef SERVER_WELCOME_DIALOG_H
#define SERVER_WELCOME_DIALOG_H

#include <QDialog>
#include <QJsonObject>
#include <QPoint>

class QLineEdit;
class QSpinBox;
class QLabel;
class QPushButton;
class QNetworkAccessManager;
class QFrame;

class ServerWelcomeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ServerWelcomeDialog(QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void onVerifyClicked();

private:
    QPoint adjustedWindowPos(const QPoint& desiredTopLeft, bool snapToEdges) const;
    bool verifyServer(QString* errorMessage);
    bool getJson(const QString& fullUrl, QJsonObject* root, QString* errorMessage);
    static QString normalizeHostInput(const QString& rawHost, int* portInOut);
    void setUiBusy(bool busy);
    void setStatusMessage(const QString& message, bool isError);

private:
    QLineEdit* m_hostEdit = nullptr;
    QSpinBox* m_portSpin = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPushButton* m_centerButton = nullptr;
    QPushButton* m_closeButton = nullptr;
    QPushButton* m_verifyButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
    QNetworkAccessManager* m_networkManager = nullptr;
    bool m_dragging = false;
    QPoint m_dragOffset;
};

#endif // SERVER_WELCOME_DIALOG_H
