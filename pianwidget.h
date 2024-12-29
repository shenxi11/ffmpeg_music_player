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
    void on_signal_set_pic_path(QString picPath);
signals:
    void signal_up_click(bool flag);
protected:
    void mousePressEvent(QMouseEvent *event) override;
private:

    QLabel *nameLabel;
    QLabel *picLabel;
};

#endif // PIANWIDGET_H
