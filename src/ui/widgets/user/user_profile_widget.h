#ifndef USER_PROFILE_WIDGET_H
#define USER_PROFILE_WIDGET_H

#include <QQuickWidget>
#include <QVariantList>
#include <QVariantMap>

class UserProfileWidget : public QQuickWidget
{
    Q_OBJECT

public:
    explicit UserProfileWidget(QWidget* parent = nullptr);

    void setProfileData(const QVariantMap& profile);
    void setStatsData(int favoritesCount, int historyCount, int playlistsCount);
    void setPreviewData(const QVariantList& favoritesPreview,
                        const QVariantList& historyPreview,
                        const QVariantList& playlistsPreview);
    void setBusy(bool busy);
    void setUsernameSaving(bool saving);
    void setAvatarUploading(bool uploading);
    void setStatusMessage(const QString& kind, const QString& text);
    void setSessionExpired(bool expired);
    void setPendingAvatarPreview(const QString& filePath);
    void clearPendingAvatarPreview();

signals:
    void refreshRequested();
    void usernameSaveRequested(const QString& username);
    void avatarFileSelected(const QString& filePath);
    void favoritesShortcutRequested();
    void historyShortcutRequested();
    void playlistsShortcutRequested();
    void reloginRequested();

private slots:
    void handleChooseAvatarRequested();
    void handleSaveUsernameRequested(const QString& username);

private:
    void invokeRootMethod(const char* method, const QVariant& arg1 = QVariant(), const QVariant& arg2 = QVariant());
};

#endif // USER_PROFILE_WIDGET_H
