#include "searchbox.h"

SearchBox::SearchBox(QWidget *parent)
    : QWidget(parent)
{
    // 移除容器背景样式，使用透明背景
    setStyleSheet(
        "SearchBox {"
        "    background: transparent;"
        "    border: none;"
        "}"
    );

    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("🎵 搜索想听的歌曲吧...");
    
    // 设置最小高度，确保文字完整显示
    searchEdit->setMinimumHeight(40);
    
    // 设置对齐方式，确保文字垂直居中
    searchEdit->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    
    searchEdit->setStyleSheet(
        "QLineEdit {"
        "    background: transparent;"
        "    border: none;"
        "    padding-left: 15px;" // 只设置左内边距
        "    padding-right: 15px;" // 只设置右内边距
        "    font-size: 14px;"
        "    color: #333;"
        "    line-height: 1.5;" // 设置行高
        "    selection-background-color: rgba(0, 122, 204, 0.3);"
        "}"
        "QLineEdit::placeholder {"
        "    color: rgba(102, 102, 102, 0.7);"
        "    font-style: italic;"
        "}"
        "QLineEdit:focus {"
        "    outline: none;"
        "}"
    );

    searchButton = new QPushButton(this);
    searchButton->setFixedSize(32, 32);
    searchButton->setStyleSheet(
        "QPushButton {"
        "    border: none;"
        "    background: transparent;"
        "    border-image: url(:/new/prefix1/icon/search.png);"
        "}"
        "QPushButton:hover {"
        "    background: rgba(0, 122, 204, 0.1);"
        "    border-radius: 16px;"
        "}"
        "QPushButton:pressed {"
        "    background: rgba(0, 122, 204, 0.2);"
        "    border-radius: 16px;"
        "}"
    );
    
    // 布局设置
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(searchEdit, 1);
    layout->addWidget(searchButton);
    layout->setContentsMargins(8, 5, 8, 5); // 保留少量上下内边距
    layout->setSpacing(10); // 增加控件间距
    layout->setAlignment(Qt::AlignVCenter); // 垂直居中对齐
    setLayout(layout);

    // 连接信号
    connect(searchButton, &QPushButton::clicked, this, &SearchBox::onSearchClicked);
    connect(searchEdit, &QLineEdit::returnPressed, this, &SearchBox::onSearchClicked);
}
void SearchBox::onSearchClicked() {
    QString text = searchEdit->text().trimmed();
    if (text.isEmpty()) {
        //QMessageBox::warning(this, "Warning", "Search term cannot be empty.");
        emit searchAll();
        return;
    }
    emit search(text);
}
