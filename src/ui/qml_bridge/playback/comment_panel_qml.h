#ifndef COMMENT_PANEL_QML_H
#define COMMENT_PANEL_QML_H

#include <QDebug>
#include <QQuickItem>
#include <QQuickWidget>
#include <QVariant>

class CommentPanelQml : public QQuickWidget {
    Q_OBJECT

  public:
    explicit CommentPanelQml(QWidget* parent = nullptr) : QQuickWidget(parent) {
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        setClearColor(Qt::transparent);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setStyleSheet(QStringLiteral("background: transparent; border: 0;"));
        setSource(QUrl(QStringLiteral("qrc:/qml/components/playback/CommentPanel.qml")));

        if (status() == QQuickWidget::Error) {
            qWarning() << "CommentPanelQml: QML loading errors:";
            for (const QQmlError& error : errors()) {
                qWarning() << " " << error.toString();
            }
        } else {
            qDebug() << "CommentPanelQml: QML loaded successfully";
        }

        QQuickItem* root = rootObject();
        if (!root) {
            qWarning() << "CommentPanelQml: root object is null";
            return;
        }

        connect(root, SIGNAL(closeRequested()), this, SIGNAL(closeRequested()));
        connect(root, SIGNAL(loadMoreCommentsRequested()), this,
                SIGNAL(loadMoreCommentsRequested()));
        connect(root, SIGNAL(toggleRepliesRequested(QVariant,bool)), this,
                SLOT(onToggleRepliesRequested(QVariant,bool)));
        connect(root, SIGNAL(loadMoreRepliesRequested(QVariant)), this,
                SLOT(onLoadMoreRepliesRequested(QVariant)));
        connect(root, SIGNAL(submitMainCommentRequested(QString)), this,
                SIGNAL(submitMainCommentRequested(QString)));
        connect(root, SIGNAL(submitReplyRequested(QVariant,QVariant,QString)), this,
                SLOT(onSubmitReplyRequested(QVariant,QVariant,QString)));
        connect(root, SIGNAL(deleteCommentRequested(QVariant)), this,
                SLOT(onDeleteCommentRequested(QVariant)));
        connect(root, SIGNAL(startReplyRequested(QVariant,QVariant,QString)), this,
                SLOT(onStartReplyRequested(QVariant,QVariant,QString)));
        connect(root, SIGNAL(loginRequested()), this, SIGNAL(loginRequested()));
    }

    void setTrackContext(const QVariantMap& context) {
        if (QQuickItem* root = rootObject()) {
            root->setProperty("trackContext", context);
        }
    }

    void setThreadMeta(const QVariantMap& meta) {
        if (QQuickItem* root = rootObject()) {
            root->setProperty("threadMeta", meta);
        }
    }

    void setDisplayMode(const QString& mode) {
        if (QQuickItem* root = rootObject()) {
            root->setProperty("displayMode", mode);
        }
    }

    void setCommentEnabled(bool enabled) {
        if (QQuickItem* root = rootObject()) {
            root->setProperty("commentEnabled", enabled);
        }
    }

    void setLoggedIn(bool loggedIn) {
        if (QQuickItem* root = rootObject()) {
            root->setProperty("loggedIn", loggedIn);
        }
    }

    void setCommentsLoading(bool loading) {
        if (QQuickItem* root = rootObject()) {
            root->setProperty("commentsLoading", loading);
        }
    }

    void setRepliesLoading(bool loading) {
        if (QQuickItem* root = rootObject()) {
            root->setProperty("repliesLoading", loading);
        }
    }

    void setSubmitting(bool submitting) {
        if (QQuickItem* root = rootObject()) {
            root->setProperty("submitting", submitting);
        }
    }

    void updateComments(const QVariantList& items, int page, bool hasMore,
                        const QString& errorMessage) {
        if (QQuickItem* root = rootObject()) {
            QMetaObject::invokeMethod(root, "updateComments", Q_ARG(QVariant, items),
                                      Q_ARG(QVariant, page), Q_ARG(QVariant, hasMore),
                                      Q_ARG(QVariant, errorMessage));
        }
    }

    void updateReplies(qint64 rootCommentId, const QVariantList& items, int page, bool hasMore,
                       int total, const QString& errorMessage) {
        if (QQuickItem* root = rootObject()) {
            QMetaObject::invokeMethod(root, "updateReplies", Q_ARG(QVariant, rootCommentId),
                                      Q_ARG(QVariant, items), Q_ARG(QVariant, page),
                                      Q_ARG(QVariant, hasMore), Q_ARG(QVariant, total),
                                      Q_ARG(QVariant, errorMessage));
        }
    }

    void setReplyTarget(qint64 rootCommentId, qint64 targetCommentId, const QString& username) {
        if (QQuickItem* root = rootObject()) {
            QMetaObject::invokeMethod(root, "setReplyTarget", Q_ARG(QVariant, rootCommentId),
                                      Q_ARG(QVariant, targetCommentId), Q_ARG(QVariant, username));
        }
    }

    void clearReplyTarget() {
        if (QQuickItem* root = rootObject()) {
            QMetaObject::invokeMethod(root, "clearReplyTarget");
        }
    }

    void clearComposer() {
        if (QQuickItem* root = rootObject()) {
            QMetaObject::invokeMethod(root, "clearComposer");
        }
    }

    void setExpandedRootCommentId(qint64 rootCommentId) {
        if (QQuickItem* root = rootObject()) {
            root->setProperty("expandedRootCommentId", rootCommentId);
        }
    }

  signals:
    void closeRequested();
    void loadMoreCommentsRequested();
    void toggleRepliesRequested(qint64 rootCommentId, bool expanded);
    void loadMoreRepliesRequested(qint64 rootCommentId);
    void submitMainCommentRequested(const QString& content);
    void submitReplyRequested(qint64 rootCommentId, qint64 targetCommentId,
                              const QString& content);
    void deleteCommentRequested(qint64 commentId);
    void startReplyRequested(qint64 rootCommentId, qint64 targetCommentId,
                             const QString& username);
    void loginRequested();

  private slots:
    void onToggleRepliesRequested(const QVariant& rootCommentId, bool expanded) {
        emit toggleRepliesRequested(rootCommentId.toLongLong(), expanded);
    }

    void onLoadMoreRepliesRequested(const QVariant& rootCommentId) {
        emit loadMoreRepliesRequested(rootCommentId.toLongLong());
    }

    void onSubmitReplyRequested(const QVariant& rootCommentId, const QVariant& targetCommentId,
                                const QString& content) {
        emit submitReplyRequested(rootCommentId.toLongLong(), targetCommentId.toLongLong(),
                                  content);
    }

    void onDeleteCommentRequested(const QVariant& commentId) {
        emit deleteCommentRequested(commentId.toLongLong());
    }

    void onStartReplyRequested(const QVariant& rootCommentId, const QVariant& targetCommentId,
                               const QString& username) {
        emit startReplyRequested(rootCommentId.toLongLong(), targetCommentId.toLongLong(),
                                 username);
    }
};

#endif // COMMENT_PANEL_QML_H
