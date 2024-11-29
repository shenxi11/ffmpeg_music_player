#include "searchbox.h"

SearchBox::SearchBox(QWidget *parent)
    : QWidget(parent)
{
    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("搜索想听的歌曲吧");

    searchButton = new QPushButton(this);
    searchButton->setStyleSheet("QPushButton {"
                                "    border-image: url(:/new/prefix1/icon/search.png);"
                                "}");
    searchButton->setFixedSize(30, 30);
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(searchEdit, 1);
    layout->addWidget(searchButton);
    layout->setContentsMargins(0, 0, 0, 0);

    connect(searchButton, &QPushButton::clicked, this, &SearchBox::onSearchClicked);
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
