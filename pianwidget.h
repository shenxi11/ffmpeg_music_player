#ifndef PIANWIDGET_H
#define PIANWIDGET_H

#include <QWidget>
#include <QHBoxLayout>
#include <QPixmap>
#include <QLabel>
#include <QPushButton>

class PianWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PianWidget(QWidget *parent = nullptr);

    void setName(const QString name);
signals:
    void signal_up_click(bool flag);
protected:
    void mousePressEvent(QMouseEvent *event) override;
private:

    QLabel *nameLabel;
};

#endif // PIANWIDGET_H
