#include "search_history_popup.h"

#include <QApplication>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

namespace {

constexpr int kRowHeight = 40;
constexpr int kListMaxHeight = 280;

void clearLayout(QVBoxLayout* layout)
{
    if (!layout) {
        return;
    }

    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
}

QString emptyStateText(const QStringList& historyItems, const QString& filterText)
{
    if (historyItems.isEmpty()) {
        return QStringLiteral("暂无搜索记录");
    }
    if (!filterText.trimmed().isEmpty()) {
        return QStringLiteral("没有匹配的搜索记录");
    }
    return QStringLiteral("暂无搜索记录");
}

} // namespace

SearchHistoryPopup::SearchHistoryPopup(QWidget* parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("SearchHistoryPopup"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFrameShape(QFrame::NoFrame);
    hide();

    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(28);
    shadow->setOffset(0, 10);
    shadow->setColor(QColor(20, 28, 40, 45));
    setGraphicsEffect(shadow);

    setStyleSheet(
        "QFrame#SearchHistoryPopup {"
        "    background: #ffffff;"
        "    border: 1px solid #E7ECF3;"
        "    border-radius: 14px;"
        "}"
        "QLabel#SearchHistoryTitle {"
        "    color: #223047;"
        "    font-size: 14px;"
        "    font-weight: 600;"
        "}"
        "QPushButton#SearchHistoryTextButton {"
        "    background: transparent;"
        "    border: none;"
        "    color: #223047;"
        "    font-size: 13px;"
        "    text-align: left;"
        "    padding: 0 2px;"
        "}"
        "QPushButton#SearchHistoryTextButton:hover {"
        "    color: #EC4141;"
        "}"
        "QPushButton#SearchHistoryDeleteButton {"
        "    background: transparent;"
        "    border: none;"
        "    color: #92A0B3;"
        "    font-size: 12px;"
        "    padding: 2px 4px;"
        "}"
        "QPushButton#SearchHistoryDeleteButton:hover {"
        "    color: #EC4141;"
        "}"
        "QPushButton#SearchHistoryClearButton {"
        "    background: transparent;"
        "    border: none;"
        "    color: #5D6B7E;"
        "    font-size: 12px;"
        "    text-align: right;"
        "    padding: 4px 0 0 0;"
        "}"
        "QPushButton#SearchHistoryClearButton:hover {"
        "    color: #EC4141;"
        "}"
        "QLabel#SearchHistoryEmptyLabel {"
        "    color: #92A0B3;"
        "    font-size: 12px;"
        "    padding: 14px 0;"
        "}");

    m_rootLayout = new QVBoxLayout(this);
    m_rootLayout->setContentsMargins(14, 14, 14, 12);
    m_rootLayout->setSpacing(10);

    m_titleLabel = new QLabel(QStringLiteral("搜索记录"), this);
    m_titleLabel->setObjectName(QStringLiteral("SearchHistoryTitle"));
    m_rootLayout->addWidget(m_titleLabel);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_rootLayout->addWidget(m_scrollArea);

    m_scrollContent = new QWidget(m_scrollArea);
    m_contentLayout = new QVBoxLayout(m_scrollContent);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->setSpacing(4);
    m_scrollArea->setWidget(m_scrollContent);

    m_clearButton = new QPushButton(QStringLiteral("清空搜索记录"), this);
    m_clearButton->setObjectName(QStringLiteral("SearchHistoryClearButton"));
    connect(m_clearButton, &QPushButton::clicked, this, &SearchHistoryPopup::clearAllRequested);
    m_rootLayout->addWidget(m_clearButton, 0, Qt::AlignRight);

    qApp->installEventFilter(this);
    rebuildContent();
}

SearchHistoryPopup::~SearchHistoryPopup()
{
    qApp->removeEventFilter(this);
}

void SearchHistoryPopup::setAnchorWidget(QWidget* anchorWidget)
{
    m_anchorWidget = anchorWidget;
}

void SearchHistoryPopup::setHistoryItems(const QStringList& items)
{
    if (m_historyItems == items) {
        return;
    }
    m_historyItems = items;
    rebuildContent();
}

void SearchHistoryPopup::setFilterText(const QString& filterText)
{
    const QString normalized = filterText.trimmed();
    if (m_filterText == normalized) {
        return;
    }
    m_filterText = normalized;
    rebuildContent();
}

QSize SearchHistoryPopup::sizeHint() const
{
    return QFrame::sizeHint().expandedTo(QSize(280, 120));
}

bool SearchHistoryPopup::eventFilter(QObject* watched, QEvent* event)
{
    Q_UNUSED(watched);
    if (!isVisible()) {
        return false;
    }

    if (event->type() == QEvent::MouseButtonPress) {
        const auto* mouseEvent = static_cast<QMouseEvent*>(event);
        const QPoint globalPos = mouseEvent->globalPos();
        if (!containsGlobalPos(globalPos) &&
            !(m_anchorWidget && QRect(m_anchorWidget->mapToGlobal(QPoint(0, 0)),
                                      m_anchorWidget->size())
                                    .contains(globalPos))) {
            hide();
        }
    }

    return false;
}

QStringList SearchHistoryPopup::filteredHistoryItems() const
{
    if (m_filterText.isEmpty()) {
        return m_historyItems;
    }

    const QString needle = m_filterText.toCaseFolded();
    QStringList filtered;
    for (const QString& keyword : m_historyItems) {
        if (keyword.toCaseFolded().contains(needle)) {
            filtered.append(keyword);
        }
    }
    return filtered;
}

void SearchHistoryPopup::rebuildContent()
{
    clearLayout(m_contentLayout);

    const QStringList filtered = filteredHistoryItems();
    if (filtered.isEmpty()) {
        auto* emptyLabel = new QLabel(emptyStateText(m_historyItems, m_filterText), m_scrollContent);
        emptyLabel->setObjectName(QStringLiteral("SearchHistoryEmptyLabel"));
        emptyLabel->setAlignment(Qt::AlignCenter);
        m_contentLayout->addWidget(emptyLabel);
        m_scrollArea->setFixedHeight(72);
    } else {
        for (const QString& keyword : filtered) {
            auto* row = new QWidget(m_scrollContent);
            row->setFixedHeight(kRowHeight);

            auto* rowLayout = new QHBoxLayout(row);
            rowLayout->setContentsMargins(0, 0, 0, 0);
            rowLayout->setSpacing(8);

            auto* textButton = new QPushButton(keyword, row);
            textButton->setObjectName(QStringLiteral("SearchHistoryTextButton"));
            textButton->setFlat(true);
            textButton->setCursor(Qt::PointingHandCursor);

            auto* deleteButton = new QPushButton(QStringLiteral("删除"), row);
            deleteButton->setObjectName(QStringLiteral("SearchHistoryDeleteButton"));
            deleteButton->setFlat(true);
            deleteButton->setCursor(Qt::PointingHandCursor);

            connect(textButton, &QPushButton::clicked, this,
                    [this, keyword]() { emit historyActivated(keyword); });
            connect(deleteButton, &QPushButton::clicked, this,
                    [this, keyword]() { emit historyDeleteRequested(keyword); });

            rowLayout->addWidget(textButton, 1);
            rowLayout->addWidget(deleteButton, 0, Qt::AlignRight);
            m_contentLayout->addWidget(row);
        }

        m_scrollArea->setFixedHeight(qMin(filtered.size() * (kRowHeight + m_contentLayout->spacing()),
                                          kListMaxHeight));
    }

    m_clearButton->setVisible(!m_historyItems.isEmpty());
    adjustSize();
}

bool SearchHistoryPopup::containsGlobalPos(const QPoint& globalPos) const
{
    return QRect(mapToGlobal(QPoint(0, 0)), size()).contains(globalPos);
}
