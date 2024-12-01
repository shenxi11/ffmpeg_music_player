#include "pianwidget.h"

PianWidget::PianWidget(QWidget *parent) : QWidget(parent)
{

    QLabel *picLabel = new QLabel(this);
    QPixmap image = QPixmap(":/new/prefix1/icon/pian.png");
    picLabel->setPixmap(image);
    picLabel->setFixedSize(100, 100);
    picLabel->setAlignment(Qt::AlignCenter);
    nameLabel = new QLabel("暂无歌曲", this);
    //nameLabel->setFixedSize(100, 30);
    nameLabel->setWordWrap(true);
    QHBoxLayout* hlayout_ = new QHBoxLayout(this);
    hlayout_->setContentsMargins(0, 0, 0, 0);
    hlayout_->addWidget(picLabel);
    hlayout_->addWidget(nameLabel);
    this->setLayout(hlayout_);
    this->setMaximumSize(200, 100);

}
void PianWidget::setName(const QString name)
{
    nameLabel->setText(name);
}
void PianWidget::mousePressEvent(QMouseEvent *event)
{
    emit signal_up_click(true);
}
