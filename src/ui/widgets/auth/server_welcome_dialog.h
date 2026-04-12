#ifndef SERVER_WELCOME_DIALOG_H
#define SERVER_WELCOME_DIALOG_H

#include <QDialog>
#include <QPoint>

class QLineEdit;
class QSpinBox;
class QLabel;
class QPushButton;
class QFrame;
class QShowEvent;
class QString;
class ServerWelcomeViewModel;

class ServerWelcomeDialog : public QDialog {
    Q_OBJECT

  public:
    explicit ServerWelcomeDialog(bool autoVerifyOnShow = true, QWidget* parent = nullptr);
    bool enterLocalOnly() const {
        return m_enterLocalOnly;
    }

  protected:
    void showEvent(QShowEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

  private slots:
    void onVerifyClicked();
    void onEnterLocalOnlyClicked();

  private:
    // 连接拆分：统一维护按钮、快捷键与输入触发行为。
    void setupInteractionConnections();
    void moveToScreenCenter();

    QPoint adjustedWindowPos(const QPoint& desiredTopLeft, bool snapToEdges) const;
    void setUiBusy(bool busy);
    void setStatusMessage(const QString& message, bool isError);

  private:
    QLineEdit* m_hostEdit = nullptr;
    QSpinBox* m_portSpin = nullptr;
    QFrame* m_statusFrame = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_localOnlyEntryLabel = nullptr;
    QPushButton* m_centerButton = nullptr;
    QPushButton* m_closeButton = nullptr;
    QPushButton* m_verifyButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
    ServerWelcomeViewModel* m_viewModel = nullptr;
    bool m_autoVerifyOnShow = true;
    bool m_autoVerifyTriggered = false;
    bool m_enterLocalOnly = false;
    bool m_dragging = false;
    QPoint m_dragOffset;
};

#endif // SERVER_WELCOME_DIALOG_H
