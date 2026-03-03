#include "desk_lrc_settings.h"
#include <QDebug>

const QColor DeskLrcSettings::DEFAULT_COLOR = QColor(0, 120, 255);  // 蓝色
const int DeskLrcSettings::DEFAULT_FONT_SIZE = 20;
const QString DeskLrcSettings::DEFAULT_FONT_FAMILY = "Microsoft YaHei";

DeskLrcSettings::DeskLrcSettings(QWidget *parent)
    : QDialog(parent)
    , lrcColor(DEFAULT_COLOR)
    , fontSize(DEFAULT_FONT_SIZE)
{
    lrcFont.setFamily(DEFAULT_FONT_FAMILY);
    lrcFont.setPointSize(fontSize);
    
    setWindowTitle("桌面歌词设置");
    setFixedSize(450, 380);
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
    
    setupUI();
    loadSettings();
    updateColorButton();
    updatePreview();
}

void DeskLrcSettings::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 预览区域
    QGroupBox *previewGroup = new QGroupBox("预览效果", this);
    QVBoxLayout *previewLayout = new QVBoxLayout(previewGroup);
    
    previewLabel = new QLabel("这是桌面歌词预览文字", this);
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setMinimumHeight(60);
    previewLabel->setStyleSheet("QLabel { background-color: #2d2d2d; border-radius: 5px; padding: 10px; }");
    previewLayout->addWidget(previewLabel);
    
    // 颜色设置
    QGroupBox *colorGroup = new QGroupBox("歌词颜色", this);
    QHBoxLayout *colorLayout = new QHBoxLayout(colorGroup);
    
    QLabel *colorLabel = new QLabel("选择颜色:", this);
    colorButton = new QPushButton(this);
    colorButton->setFixedSize(60, 30);
    colorButton->setText("选择");
    
    colorLayout->addWidget(colorLabel);
    colorLayout->addWidget(colorButton);
    colorLayout->addStretch();
    
    // 字体设置
    QGroupBox *fontGroup = new QGroupBox("字体设置", this);
    QVBoxLayout *fontLayout = new QVBoxLayout(fontGroup);
    
    // 字体选择
    QHBoxLayout *fontSelectLayout = new QHBoxLayout();
    QLabel *fontLabel = new QLabel("字体:", this);
    fontButton = new QPushButton("选择字体", this);
    fontButton->setFixedSize(100, 30);
    fontNameLabel = new QLabel(lrcFont.family(), this);
    fontNameLabel->setStyleSheet("QLabel { color: #666; font-style: italic; }");
    
    fontSelectLayout->addWidget(fontLabel);
    fontSelectLayout->addWidget(fontButton);
    fontSelectLayout->addWidget(fontNameLabel);
    fontSelectLayout->addStretch();
    
    // 字号设置
    QHBoxLayout *sliderLayout = new QHBoxLayout();
    QLabel *sizeLabel = new QLabel("字号:", this);
    QLabel *minLabel = new QLabel("12", this);
    fontSizeSlider = new QSlider(Qt::Horizontal, this);
    fontSizeSlider->setRange(12, 48);
    fontSizeSlider->setValue(fontSize);
    QLabel *maxLabel = new QLabel("48", this);
    
    sliderLayout->addWidget(sizeLabel);
    sliderLayout->addWidget(minLabel);
    sliderLayout->addWidget(fontSizeSlider);
    sliderLayout->addWidget(maxLabel);
    
    fontSizeLabel = new QLabel(QString("当前大小: %1").arg(fontSize), this);
    fontSizeLabel->setAlignment(Qt::AlignCenter);
    
    fontLayout->addLayout(fontSelectLayout);
    fontLayout->addLayout(sliderLayout);
    fontLayout->addWidget(fontSizeLabel);
    
    // 按钮区域
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    applyButton = new QPushButton("应用", this);
    resetButton = new QPushButton("重置", this);
    closeButton = new QPushButton("关闭", this);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(applyButton);
    buttonLayout->addWidget(resetButton);
    buttonLayout->addWidget(closeButton);
    
    // 添加到主布局
    mainLayout->addWidget(previewGroup);
    mainLayout->addWidget(colorGroup);
    mainLayout->addWidget(fontGroup);
    mainLayout->addLayout(buttonLayout);
    
    // 连接信号
    connect(colorButton, &QPushButton::clicked, this, &DeskLrcSettings::onColorButtonClicked);
    connect(fontButton, &QPushButton::clicked, this, &DeskLrcSettings::onFontButtonClicked);
    connect(fontSizeSlider, &QSlider::valueChanged, this, &DeskLrcSettings::onFontSizeChanged);
    connect(applyButton, &QPushButton::clicked, this, &DeskLrcSettings::onApplyClicked);
    connect(resetButton, &QPushButton::clicked, this, &DeskLrcSettings::onResetClicked);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

void DeskLrcSettings::setLrcColor(const QColor &color)
{
    lrcColor = color;
    updateColorButton();
    updatePreview();
}

void DeskLrcSettings::setFontSize(int size)
{
    fontSize = size;
    lrcFont.setPointSize(size);
    fontSizeSlider->setValue(size);
    fontSizeLabel->setText(QString("当前大小: %1").arg(size));
    updatePreview();
}

void DeskLrcSettings::setLrcFont(const QFont &font)
{
    lrcFont = font;
    fontSize = font.pointSize();
    fontSizeSlider->setValue(fontSize);
    fontSizeLabel->setText(QString("当前大小: %1").arg(fontSize));
    fontNameLabel->setText(font.family());
    updatePreview();
}

void DeskLrcSettings::onColorButtonClicked()
{
    QColor color = QColorDialog::getColor(lrcColor, this, "选择歌词颜色");
    if (color.isValid()) {
        lrcColor = color;
        updateColorButton();
        updatePreview();
    }
}

void DeskLrcSettings::onFontSizeChanged(int size)
{
    fontSize = size;
    lrcFont.setPointSize(size);
    fontSizeLabel->setText(QString("当前大小: %1").arg(size));
    updatePreview();
}

void DeskLrcSettings::onFontButtonClicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, lrcFont, this, "选择字体");
    if (ok) {
        lrcFont = font;
        fontSize = font.pointSize();
        fontSizeSlider->setValue(fontSize);
        fontSizeLabel->setText(QString("当前大小: %1").arg(fontSize));
        fontNameLabel->setText(font.family());
        updatePreview();
    }
}

void DeskLrcSettings::onApplyClicked()
{
    saveSettings();
    emit settingsChanged(lrcColor, fontSize, lrcFont);
    accept();
}

void DeskLrcSettings::onResetClicked()
{
    lrcColor = DEFAULT_COLOR;
    fontSize = DEFAULT_FONT_SIZE;
    lrcFont.setFamily(DEFAULT_FONT_FAMILY);
    lrcFont.setPointSize(fontSize);
    
    fontSizeSlider->setValue(fontSize);
    fontNameLabel->setText(lrcFont.family());
    updateColorButton();
    updatePreview();
}

void DeskLrcSettings::updateColorButton()
{
    QString styleSheet = QString("QPushButton { background-color: %1; border: 1px solid #999; border-radius: 3px; }")
                         .arg(lrcColor.name());
    colorButton->setStyleSheet(styleSheet);
}

void DeskLrcSettings::updatePreview()
{
    previewLabel->setFont(lrcFont);
    previewLabel->setStyleSheet(QString("QLabel { color: %1; background-color: #2d2d2d; border-radius: 5px; padding: 10px; }")
                               .arg(lrcColor.name()));
}

void DeskLrcSettings::loadSettings()
{
    QSettings settings;
    lrcColor = settings.value("DeskLrc/color", DEFAULT_COLOR).value<QColor>();
    fontSize = settings.value("DeskLrc/fontSize", DEFAULT_FONT_SIZE).toInt();
    QString fontFamily = settings.value("DeskLrc/fontFamily", DEFAULT_FONT_FAMILY).toString();
    
    qDebug() << "Settings loaded - Color:" << lrcColor.name() 
             << "Font:" << fontFamily 
             << "Size:" << fontSize;
    
    if (fontSize < 12) fontSize = 12;
    if (fontSize > 48) fontSize = 48;
    
    lrcFont.setFamily(fontFamily);
    lrcFont.setPointSize(fontSize);
    
    // 更新UI控件显示
    fontSizeSlider->setValue(fontSize);
    fontSizeLabel->setText(QString("当前大小: %1").arg(fontSize));
    fontNameLabel->setText(fontFamily);
}

void DeskLrcSettings::saveSettings()
{
    QSettings settings;
    settings.setValue("DeskLrc/color", lrcColor);
    settings.setValue("DeskLrc/fontSize", fontSize);
    settings.setValue("DeskLrc/fontFamily", lrcFont.family());
    settings.sync(); // 强制写入磁盘
    
    qDebug() << "Settings saved - Color:" << lrcColor.name() 
             << "Font:" << lrcFont.family() 
             << "Size:" << fontSize;
}

void DeskLrcSettings::refreshFromCurrentSettings()
{
    loadSettings();
    updateColorButton();
    updatePreview();
}
