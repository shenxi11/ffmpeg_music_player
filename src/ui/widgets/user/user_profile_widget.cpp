#include "user_profile_widget.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QQuickItem>
#include <QUrl>

namespace {
const qint64 kMaxAvatarBytes = 5 * 1024 * 1024;
const QStringList kSupportedAvatarExtensions = {QStringLiteral("jpg"), QStringLiteral("jpeg"),
                                                QStringLiteral("png"), QStringLiteral("webp")};
} // namespace

UserProfileWidget::UserProfileWidget(QWidget* parent) : QQuickWidget(parent) {
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setSource(QUrl(QStringLiteral("qrc:/qml/components/user/UserProfilePage.qml")));
    setClearColor(Qt::transparent);

    if (QQuickItem* root = rootObject()) {
        connect(root, SIGNAL(refreshRequested()), this, SIGNAL(refreshRequested()));
        connect(root, SIGNAL(saveUsernameRequested(QString)), this,
                SLOT(handleSaveUsernameRequested(QString)));
        connect(root, SIGNAL(chooseAvatarRequested()), this, SLOT(handleChooseAvatarRequested()));
        connect(root, SIGNAL(favoritesShortcutRequested()), this,
                SIGNAL(favoritesShortcutRequested()));
        connect(root, SIGNAL(historyShortcutRequested()), this, SIGNAL(historyShortcutRequested()));
        connect(root, SIGNAL(playlistsShortcutRequested()), this,
                SIGNAL(playlistsShortcutRequested()));
        connect(root, SIGNAL(reloginRequested()), this, SIGNAL(reloginRequested()));
    }
}

void UserProfileWidget::setProfileData(const QVariantMap& profile) {
    invokeRootMethod("setProfileData", profile);
}

void UserProfileWidget::setStatsData(int favoritesCount, int historyCount, int playlistsCount) {
    if (QQuickItem* root = rootObject()) {
        QMetaObject::invokeMethod(root, "setStatsData", Q_ARG(QVariant, QVariant(favoritesCount)),
                                  Q_ARG(QVariant, QVariant(historyCount)),
                                  Q_ARG(QVariant, QVariant(playlistsCount)));
    }
}

void UserProfileWidget::setPreviewData(const QVariantList& favoritesPreview,
                                       const QVariantList& historyPreview,
                                       const QVariantList& playlistsPreview) {
    if (QQuickItem* root = rootObject()) {
        QMetaObject::invokeMethod(root, "setPreviewData",
                                  Q_ARG(QVariant, QVariant::fromValue(favoritesPreview)),
                                  Q_ARG(QVariant, QVariant::fromValue(historyPreview)),
                                  Q_ARG(QVariant, QVariant::fromValue(playlistsPreview)));
    }
}

void UserProfileWidget::setBusy(bool busy) {
    invokeRootMethod("setBusyState", busy);
}

void UserProfileWidget::setUsernameSaving(bool saving) {
    invokeRootMethod("setUsernameSavingState", saving);
}

void UserProfileWidget::setAvatarUploading(bool uploading) {
    invokeRootMethod("setAvatarUploadingState", uploading);
}

void UserProfileWidget::setStatusMessage(const QString& kind, const QString& text) {
    if (QQuickItem* root = rootObject()) {
        QMetaObject::invokeMethod(root, "setStatusMessage", Q_ARG(QVariant, QVariant(kind)),
                                  Q_ARG(QVariant, QVariant(text)));
    }
}

void UserProfileWidget::setSessionExpired(bool expired) {
    invokeRootMethod("setSessionExpiredState", expired);
}

void UserProfileWidget::setPendingAvatarPreview(const QString& filePath) {
    const QString previewSource = QUrl::fromLocalFile(filePath).toString();
    invokeRootMethod("setPendingAvatarPreview", previewSource);
}

void UserProfileWidget::clearPendingAvatarPreview() {
    invokeRootMethod("clearPendingAvatarPreview");
}

void UserProfileWidget::handleChooseAvatarRequested() {
    const QString filePath =
        QFileDialog::getOpenFileName(this, QStringLiteral("选择头像"), QString(),
                                     QStringLiteral("图片文件 (*.jpg *.jpeg *.png *.webp)"));
    if (filePath.trimmed().isEmpty()) {
        return;
    }

    const QFileInfo info(filePath);
    const QString extension = info.suffix().trimmed().toLower();
    if (!kSupportedAvatarExtensions.contains(extension)) {
        setStatusMessage(QStringLiteral("error"),
                         QStringLiteral("头像格式仅支持 jpg/jpeg/png/webp。"));
        return;
    }

    if (info.size() > kMaxAvatarBytes) {
        setStatusMessage(QStringLiteral("error"), QStringLiteral("头像文件不能超过 5MB。"));
        return;
    }

    setPendingAvatarPreview(filePath);
    emit avatarFileSelected(filePath);
}

void UserProfileWidget::handleSaveUsernameRequested(const QString& username) {
    emit usernameSaveRequested(username);
}

void UserProfileWidget::invokeRootMethod(const char* method, const QVariant& arg1,
                                         const QVariant& arg2) {
    if (QQuickItem* root = rootObject()) {
        if (arg1.isValid() && arg2.isValid()) {
            QMetaObject::invokeMethod(root, method, Q_ARG(QVariant, arg1), Q_ARG(QVariant, arg2));
        } else if (arg1.isValid()) {
            QMetaObject::invokeMethod(root, method, Q_ARG(QVariant, arg1));
        } else {
            QMetaObject::invokeMethod(root, method);
        }
    }
}
