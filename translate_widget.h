
#ifndef TRANSLATEWIDGET_H
#define TRANSLATEWIDGET_H


#include <QObject>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QFileDialog>
#include <QComboBox>
#include <QProgressBar>

class TranslateWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TranslateWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;


private:
    QLabel* titleLabel;
    QLabel* fileLabel;
    QLineEdit* filePathEdit;
    QPushButton* browseButton;
    QLabel* modelLabel;
    QComboBox* modelCombo;
    QLabel* formatLabel;
    QComboBox* formatCombo;
    QLabel* langLabel;
    QComboBox* langCombo;
    QPushButton* transcribeButton;
    QTextEdit* resultEdit;
    QProgressBar* progressBar;
    QVBoxLayout* mainLayout;
    QHBoxLayout* fileLayout;
    QHBoxLayout* modelLayout;
    QHBoxLayout* formatLayout;
    QHBoxLayout* langLayout;
    QHBoxLayout* titleBarLayout;
    QWidget* titleBarWidget;
    QPushButton* minimizeButton;
    QPushButton* maximizeButton;
    QPushButton* closeButton;

    bool mousePressed = false;
    QPoint mouseStartPoint;
    QPoint windowStartPoint;
signals:

};

#endif // TRANSLATEWIDGET_H
