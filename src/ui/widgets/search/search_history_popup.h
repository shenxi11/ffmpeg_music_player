#ifndef SEARCH_HISTORY_POPUP_H
#define SEARCH_HISTORY_POPUP_H

#include <QFrame>
#include <QStringList>

class QLabel;
class QPushButton;
class QScrollArea;
class QVBoxLayout;
class QWidget;

class SearchHistoryPopup : public QFrame
{
    Q_OBJECT

public:
    explicit SearchHistoryPopup(QWidget* parent = nullptr);
    ~SearchHistoryPopup() override;

    void setAnchorWidget(QWidget* anchorWidget);
    void setHistoryItems(const QStringList& items);
    void setFilterText(const QString& filterText);
    QSize sizeHint() const override;

signals:
    void historyActivated(const QString& keyword);
    void historyDeleteRequested(const QString& keyword);
    void clearAllRequested();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QStringList filteredHistoryItems() const;
    void rebuildContent();
    bool containsGlobalPos(const QPoint& globalPos) const;

    QWidget* m_anchorWidget = nullptr;
    QStringList m_historyItems;
    QString m_filterText;
    QVBoxLayout* m_rootLayout = nullptr;
    QLabel* m_titleLabel = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_scrollContent = nullptr;
    QVBoxLayout* m_contentLayout = nullptr;
    QPushButton* m_clearButton = nullptr;
};

#endif // SEARCH_HISTORY_POPUP_H
