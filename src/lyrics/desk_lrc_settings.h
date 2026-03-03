#ifndef DESKLRCSETTINGS_H
#define DESKLRCSETTINGS_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QColorDialog>
#include <QGroupBox>
#include <QSlider>
#include <QFont>
#include <QSettings>
#include <QFontComboBox>
#include <QFontDialog>

class DeskLrcSettings : public QDialog
{
    Q_OBJECT

public:
    explicit DeskLrcSettings(QWidget *parent = nullptr);
    
    QColor getLrcColor() const { return lrcColor; }
    int getFontSize() const { return fontSize; }
    QFont getLrcFont() const { return lrcFont; }
    
    void setLrcColor(const QColor &color);
    void setFontSize(int size);
    void setLrcFont(const QFont &font);
    void refreshFromCurrentSettings();

signals:
    void settingsChanged(const QColor &color, int fontSize, const QFont &font);

private slots:
    void onColorButtonClicked();
    void onFontSizeChanged(int size);
    void onFontButtonClicked();
    void onApplyClicked();
    void onResetClicked();

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void updateColorButton();
    void updatePreview();

    QLabel *previewLabel;
    QPushButton *colorButton;
    QPushButton *fontButton;
    QSlider *fontSizeSlider;
    QLabel *fontSizeLabel;
    QLabel *fontNameLabel;
    QPushButton *applyButton;
    QPushButton *resetButton;
    QPushButton *closeButton;
    
    QColor lrcColor;
    int fontSize;
    QFont lrcFont;
    
    // 默认值
    static const QColor DEFAULT_COLOR;
    static const int DEFAULT_FONT_SIZE;
    static const QString DEFAULT_FONT_FAMILY;
};

#endif // DESKLRCSETTINGS_H
