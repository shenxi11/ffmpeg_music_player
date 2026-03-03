#ifndef SEARCHBOX_H
#define SEARCHBOX_H

#include <QObject>
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMessageBox>

class SearchBox : public QWidget {
    Q_OBJECT

public:
    explicit SearchBox(QWidget *parent = nullptr);

signals:
    void search(const QString &text);
    void searchAll();
private slots:
    void onSearchClicked();

private:
    QLineEdit *searchEdit;
    QPushButton *searchButton;
};

#endif // SEARCHBOX_H
