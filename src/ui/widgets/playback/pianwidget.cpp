#include "pianwidget.h"

PianWidget::PianWidget(QWidget *parent) : QWidget(parent)
{

    picLabel = new QLabel(this);
    QIcon defaultCoverIcon(QStringLiteral(":/qml/assets/ai/icons/default-music-cover.svg"));
    picLabel->setPixmap(defaultCoverIcon.pixmap(100, 100));
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
    emit signalUpClick(true);
}
void PianWidget::onSetPicPath(QString picPath)
{
    QPixmap image = QPixmap(picPath);
    if (image.isNull()) {
        QIcon defaultCoverIcon(QStringLiteral(":/qml/assets/ai/icons/default-music-cover.svg"));
        picLabel->setPixmap(defaultCoverIcon.pixmap(100, 100));
        return;
    }
    picLabel->setPixmap(image);
}
