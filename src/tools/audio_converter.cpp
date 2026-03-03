#include "audio_converter.h"

AudioConverter::AudioConverter(QWidget *parent)
    : QWidget(parent), currentConversionIndex(-1)
{
    // 设置为独立窗口
    setWindowFlags(Qt::Window);
    setWindowTitle("音频转换器");
    setWindowIcon(QIcon(":/icon/Music.png")); // 如果有图标的话
    
    // 设置窗口大小和位置
    resize(900, 700);
    
    // 居中显示
    if (parent) {
        QRect parentGeometry = parent->geometry();
        move(parentGeometry.center() - rect().center());
    }
    
    setAcceptDrops(true);
    
    // 设置FFmpeg路径
    ffmpegPath = "E:/ffmpeg-4.4/bin/ffmpeg.exe";
    
    // 设置默认输出目录
    outputDirectory = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    
    // 初始化进程
    ffmpegProcess = new QProcess(this);
    connect(ffmpegProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &AudioConverter::onConversionFinished);
    
    setupUI();
}

void AudioConverter::setupUI()
{
    // 设置整体样式
    setStyleSheet(
        "AudioConverter {"
        "    background: #f8f9fa;"
        "    border: 1px solid #e0e0e0;"
        "}"
        "QLabel {"
        "    color: #333;"
        "    font-family: 'Microsoft YaHei';"
        "}"
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                              stop:0 #ffffff, stop:1 #f0f0f0);"
        "    border: 1px solid #ccc;"
        "    border-radius: 6px;"
        "    padding: 8px 16px;"
        "    font-family: 'Microsoft YaHei';"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                              stop:0 #e6f3ff, stop:1 #d1e7f0);"
        "    border: 1px solid #007acc;"
        "}"
        "QPushButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                              stop:0 #d1e7f0, stop:1 #b3d9f2);"
        "    border: 1px solid #005fa3;"
        "}"
    );
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);
    
    // 标题
    QLabel* titleLabel = new QLabel("音频转换 (支持工具不用于QMCx,kgmx,krc,tkm,mfac,mgm格式的转换)", this);
    titleLabel->setStyleSheet(
        "QLabel {"
        "    color: #333;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "    padding: 10px;"
        "    background: #e8f4fd;"
        "    border-radius: 8px;"
        "}"
    );
    titleLabel->setAlignment(Qt::AlignCenter);
    
    // 操作按钮行
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    addAudioBtn = new QPushButton("添加音频", this);
    importCDBtn = new QPushButton("导入CD", this);
    clearCompletedBtn = new QPushButton("清除已完", this);
    
    addAudioBtn->setFixedSize(80, 35);
    importCDBtn->setFixedSize(80, 35);
    clearCompletedBtn->setFixedSize(80, 35);
    
    QString btnStyle = createButtonStyle();
    addAudioBtn->setStyleSheet(btnStyle);
    importCDBtn->setStyleSheet(btnStyle);
    clearCompletedBtn->setStyleSheet(btnStyle);
    
    buttonLayout->addWidget(addAudioBtn);
    buttonLayout->addWidget(importCDBtn);
    buttonLayout->addWidget(clearCompletedBtn);
    buttonLayout->addStretch();
    
    // 文件列表
    setupTable();
    
    // 转换设置
    QWidget* settingsWidget = new QWidget(this);
    settingsWidget->setStyleSheet(
        "QWidget {"
        "    background: white;"
        "    border: 1px solid #ddd;"
        "    border-radius: 8px;"
        "    padding: 15px;"
        "}"
    );
    
    QHBoxLayout* settingsLayout = new QHBoxLayout(settingsWidget);
    
    // 转换为
    QLabel* formatLabel = new QLabel("转换为", this);
    formatCombo = new QComboBox(this);
    formatCombo->addItems({"MP3", "WAV", "FLAC", "AAC", "OGG"});
    formatCombo->setFixedSize(80, 30);
    formatCombo->setStyleSheet(createComboStyle());
    
    // 编码器
    QLabel* encoderLabel = new QLabel("编码器", this);
    encoderCombo = new QComboBox(this);
    encoderCombo->addItems({"可变码率 V0", "可变码率 V2", "恒定码率"});
    encoderCombo->setCurrentText("可变码率 V0");
    encoderCombo->setFixedSize(120, 30);
    encoderCombo->setStyleSheet(createComboStyle());
    
    // 码率
    QLabel* bitrateLabel = new QLabel("恒定码率", this);
    bitrateCombo = new QComboBox(this);
    bitrateCombo->addItems({"128", "192", "256", "320"});
    bitrateCombo->setCurrentText("320");
    bitrateCombo->setFixedSize(80, 30);
    bitrateCombo->setStyleSheet(createComboStyle());
    
    settingsLayout->addWidget(formatLabel);
    settingsLayout->addWidget(formatCombo);
    settingsLayout->addSpacing(20);
    settingsLayout->addWidget(encoderLabel);
    settingsLayout->addWidget(encoderCombo);
    settingsLayout->addSpacing(20);
    settingsLayout->addWidget(bitrateLabel);
    settingsLayout->addWidget(bitrateCombo);
    settingsLayout->addStretch();
    
    // 保存位置
    QWidget* pathWidget = new QWidget(this);
    pathWidget->setStyleSheet(
        "QWidget {"
        "    background: white;"
        "    border: 1px solid #ddd;"
        "    border-radius: 8px;"
        "    padding: 15px;"
        "}"
    );
    
    QHBoxLayout* pathLayout = new QHBoxLayout(pathWidget);
    
    QLabel* pathLabel = new QLabel("保存到", this);
    outputDirEdit = new QLineEdit(outputDirectory, this);
    outputDirEdit->setReadOnly(true);
    outputDirEdit->setStyleSheet(
        "QLineEdit {"
        "    border: 1px solid #ccc;"
        "    border-radius: 4px;"
        "    padding: 6px;"
        "    background: #f9f9f9;"
        "}"
    );
    
    changeDirBtn = new QPushButton("更改目录", this);
    openFolderBtn = new QPushButton("打开文件夹", this);
    changeDirBtn->setFixedSize(80, 32);
    openFolderBtn->setFixedSize(100, 32);
    changeDirBtn->setStyleSheet(btnStyle);
    openFolderBtn->setStyleSheet(btnStyle);
    
    pathLayout->addWidget(pathLabel);
    pathLayout->addWidget(outputDirEdit);
    pathLayout->addWidget(changeDirBtn);
    pathLayout->addWidget(openFolderBtn);
    
    // 转换控制
    QHBoxLayout* controlLayout = new QHBoxLayout();
    
    startConversionBtn = new QPushButton("开始转换", this);
    startConversionBtn->setFixedSize(120, 40);
    startConversionBtn->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                              stop:0 #4CAF50, stop:1 #45a049);"
        "    color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                              stop:0 #5CBF60, stop:1 #4CAF50);"
        "}"
        "QPushButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                              stop:0 #45a049, stop:1 #3d8b40);"
        "}"
        "QPushButton:disabled {"
        "    background: #cccccc;"
        "    color: #666666;"
        "}"
    );
    
    progressBar = new QProgressBar(this);
    progressBar->setVisible(false);
    progressBar->setStyleSheet(
        "QProgressBar {"
        "    border: 1px solid #ccc;"
        "    border-radius: 8px;"
        "    background: #f0f0f0;"
        "    text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                              stop:0 #4CAF50, stop:1 #45a049);"
        "    border-radius: 7px;"
        "}"
    );
    
    statusLabel = new QLabel("就绪", this);
    statusLabel->setStyleSheet("color: #666; font-size: 12px;");
    
    controlLayout->addWidget(startConversionBtn);
    controlLayout->addWidget(progressBar);
    controlLayout->addStretch();
    controlLayout->addWidget(statusLabel);
    
    // 添加到主布局
    mainLayout->addWidget(titleLabel);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(fileTable);
    mainLayout->addWidget(settingsWidget);
    mainLayout->addWidget(pathWidget);
    mainLayout->addLayout(controlLayout);
    
    // 连接信号
    connect(addAudioBtn, &QPushButton::clicked, this, &AudioConverter::onAddAudioClicked);
    connect(importCDBtn, &QPushButton::clicked, this, &AudioConverter::onImportCDClicked);
    connect(clearCompletedBtn, &QPushButton::clicked, this, &AudioConverter::onClearCompletedClicked);
    connect(changeDirBtn, &QPushButton::clicked, this, &AudioConverter::onChangeDirClicked);
    connect(openFolderBtn, &QPushButton::clicked, this, &AudioConverter::onOpenFolderClicked);
    connect(startConversionBtn, &QPushButton::clicked, this, &AudioConverter::onStartConversionClicked);
    connect(formatCombo, &QComboBox::currentTextChanged, this, &AudioConverter::onFormatChanged);
    connect(encoderCombo, &QComboBox::currentTextChanged, this, &AudioConverter::onEncoderChanged);
    connect(bitrateCombo, &QComboBox::currentTextChanged, this, &AudioConverter::onBitrateChanged);
}

void AudioConverter::setupTable()
{
    fileTable = new QTableWidget(0, 3, this);
    fileTable->setHorizontalHeaderLabels({"文件名", "时长", "状态"});
    fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    fileTable->setAlternatingRowColors(true);
    fileTable->setStyleSheet(createTableStyle());
    fileTable->setMinimumHeight(200);
    
    // 设置列宽
    fileTable->horizontalHeader()->setStretchLastSection(true);
    fileTable->setColumnWidth(0, 300);
    fileTable->setColumnWidth(1, 80);
    
    // 设置表头样式
    fileTable->horizontalHeader()->setStyleSheet(
        "QHeaderView::section {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                              stop:0 #f0f0f0, stop:1 #e0e0e0);"
        "    border: 1px solid #c0c0c0;"
        "    padding: 8px;"
        "    font-weight: bold;"
        "}"
    );
}

QString AudioConverter::createButtonStyle()
{
    return QString(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                              stop:0 #ffffff, stop:1 #f0f0f0);"
        "    border: 1px solid #ccc;"
        "    border-radius: 4px;"
        "    padding: 6px 12px;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                              stop:0 #e8f4fd, stop:1 #d1e7f0);"
        "    border: 1px solid #007acc;"
        "}"
        "QPushButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                              stop:0 #d1e7f0, stop:1 #b3d9f2);"
        "}"
    );
}

QString AudioConverter::createComboStyle()
{
    return QString(
        "QComboBox {"
        "    border: 1px solid #ccc;"
        "    border-radius: 4px;"
        "    padding: 4px 8px;"
        "    background: white;"
        "}"
        "QComboBox::drop-down {"
        "    border: none;"
        "    width: 20px;"
        "}"
        "QComboBox::down-arrow {"
        "    width: 12px;"
        "    height: 12px;"
        "}"
    );
}

QString AudioConverter::createTableStyle()
{
    return QString(
        "QTableWidget {"
        "    background: white;"
        "    border: 1px solid #ddd;"
        "    border-radius: 8px;"
        "    gridline-color: #f0f0f0;"
        "}"
        "QTableWidget::item {"
        "    padding: 8px;"
        "    border-bottom: 1px solid #f0f0f0;"
        "}"
        "QTableWidget::item:selected {"
        "    background: #e8f4fd;"
        "}"
    );
}

void AudioConverter::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        for (const QUrl &url : urls) {
            QString filePath = url.toLocalFile();
            QFileInfo fileInfo(filePath);
            QStringList supportedFormats = {"mp3", "wav", "flac", "aac", "ogg", "m4a", "wma"};
            if (supportedFormats.contains(fileInfo.suffix().toLower())) {
                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}

void AudioConverter::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl &url : urls) {
        QString filePath = url.toLocalFile();
        QFileInfo fileInfo(filePath);
        QStringList supportedFormats = {"mp3", "wav", "flac", "aac", "ogg", "m4a", "wma"};
        if (supportedFormats.contains(fileInfo.suffix().toLower())) {
            addAudioFile(filePath);
        }
    }
    event->acceptProposedAction();
}

void AudioConverter::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(248, 249, 250));
}

void AudioConverter::onAddAudioClicked()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        "选择音频文件",
        QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
        "音频文件 (*.mp3 *.wav *.flac *.aac *.ogg *.m4a *.wma)"
    );
    
    for (const QString &file : files) {
        addAudioFile(file);
    }
}

void AudioConverter::onImportCDClicked()
{
    QMessageBox::information(this, "提示", "CD导入功能暂未实现");
}

void AudioConverter::onClearCompletedClicked()
{
    for (int row = fileTable->rowCount() - 1; row >= 0; --row) {
        QTableWidgetItem* statusItem = fileTable->item(row, 2);
        if (statusItem && statusItem->text() == "已完成") {
            fileTable->removeRow(row);
        }
    }
}

void AudioConverter::onChangeDirClicked()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "选择输出目录",
        outputDirectory
    );
    
    if (!dir.isEmpty()) {
        outputDirectory = dir;
        outputDirEdit->setText(outputDirectory);
    }
}

void AudioConverter::onOpenFolderClicked()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(outputDirectory));
}

void AudioConverter::onStartConversionClicked()
{
    if (fileTable->rowCount() == 0) {
        QMessageBox::warning(this, "警告", "请先添加音频文件");
        return;
    }
    
    // 检查FFmpeg是否存在
    if (!QFile::exists(ffmpegPath)) {
        QMessageBox::critical(this, "错误", 
            QString("找不到FFmpeg程序：%1\n请确认路径是否正确").arg(ffmpegPath));
        return;
    }
    
    // 准备转换队列
    conversionQueue.clear();
    for (int row = 0; row < fileTable->rowCount(); ++row) {
        QTableWidgetItem* statusItem = fileTable->item(row, 2);
        if (statusItem && statusItem->text() != "已完成") {
            conversionQueue.append(fileTable->item(row, 0)->data(Qt::UserRole).toString());
            statusItem->setText("等待中");
        }
    }
    
    if (conversionQueue.isEmpty()) {
        QMessageBox::information(this, "提示", "没有需要转换的文件");
        return;
    }
    
    currentConversionIndex = -1;
    progressBar->setMaximum(conversionQueue.size());
    progressBar->setValue(0);
    progressBar->setVisible(true);
    startConversionBtn->setEnabled(false);
    
    startNextConversion();
}

void AudioConverter::onFormatChanged()
{
    // 根据格式更新编码器选项
    QString format = formatCombo->currentText();
    encoderCombo->clear();
    
    if (format == "MP3") {
        encoderCombo->addItems({"可变码率 V0", "可变码率 V2", "恒定码率"});
    } else if (format == "AAC") {
        encoderCombo->addItems({"AAC-LC", "AAC-HE", "AAC-HEv2"});
    } else {
        encoderCombo->addItems({"默认编码器"});
    }
}

void AudioConverter::onEncoderChanged()
{
    // 根据编码器启用/禁用码率选择
    QString encoder = encoderCombo->currentText();
    bitrateCombo->setEnabled(encoder.contains("恒定码率") || encoder.contains("AAC"));
}

void AudioConverter::onBitrateChanged()
{
    // 码率改变处理
}

void AudioConverter::addAudioFile(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    
    // 检查文件是否已存在
    for (int row = 0; row < fileTable->rowCount(); ++row) {
        if (fileTable->item(row, 0)->data(Qt::UserRole).toString() == filePath) {
            return; // 文件已存在
        }
    }
    
    int row = fileTable->rowCount();
    fileTable->insertRow(row);
    
    // 文件名
    QTableWidgetItem* nameItem = new QTableWidgetItem(fileInfo.fileName());
    nameItem->setData(Qt::UserRole, filePath);
    fileTable->setItem(row, 0, nameItem);
    
    // 时长
    QTime duration = getAudioDuration(filePath);
    QTableWidgetItem* durationItem = new QTableWidgetItem(duration.toString("mm:ss"));
    fileTable->setItem(row, 1, durationItem);
    
    // 状态
    QTableWidgetItem* statusItem = new QTableWidgetItem("等待转换");
    fileTable->setItem(row, 2, statusItem);
}

void AudioConverter::startNextConversion()
{
    currentConversionIndex++;
    
    if (currentConversionIndex >= conversionQueue.size()) {
        // 所有转换完成
        progressBar->setVisible(false);
        startConversionBtn->setEnabled(true);
        statusLabel->setText("转换完成");
        QMessageBox::information(this, "完成", "所有文件转换完成！");
        return;
    }
    
    QString inputFile = conversionQueue[currentConversionIndex];
    QString outputFile = getOutputPath(inputFile);
    
    // 更新状态
    for (int row = 0; row < fileTable->rowCount(); ++row) {
        if (fileTable->item(row, 0)->data(Qt::UserRole).toString() == inputFile) {
            fileTable->item(row, 2)->setText("转换中...");
            break;
        }
    }
    
    progressBar->setValue(currentConversionIndex);
    statusLabel->setText(QString("正在转换: %1").arg(QFileInfo(inputFile).fileName()));
    
    // 构建FFmpeg命令
    QStringList arguments;
    arguments << "-i" << inputFile;
    
    QString format = formatCombo->currentText().toLower();
    QString encoder = encoderCombo->currentText();
    
    if (format == "mp3") {
        arguments << "-acodec" << "libmp3lame";
        if (encoder.contains("可变码率 V0")) {
            arguments << "-q:a" << "0";
        } else if (encoder.contains("可变码率 V2")) {
            arguments << "-q:a" << "2";
        } else {
            arguments << "-b:a" << bitrateCombo->currentText() + "k";
        }
    } else if (format == "aac") {
        arguments << "-acodec" << "aac";
        arguments << "-b:a" << bitrateCombo->currentText() + "k";
    } else if (format == "wav") {
        arguments << "-acodec" << "pcm_s16le";
    } else if (format == "flac") {
        arguments << "-acodec" << "flac";
    } else if (format == "ogg") {
        arguments << "-acodec" << "libvorbis";
        arguments << "-q:a" << "5";
    }
    
    arguments << "-y" << outputFile;
    
    ffmpegProcess->start(ffmpegPath, arguments);
}

QString AudioConverter::getOutputPath(const QString &inputPath)
{
    QFileInfo inputInfo(inputPath);
    QString format = formatCombo->currentText().toLower();
    QString outputFileName = inputInfo.baseName() + "." + format;
    return QDir(outputDirectory).filePath(outputFileName);
}

QTime AudioConverter::getAudioDuration(const QString &filePath)
{
    // 使用FFprobe获取音频时长（简化实现，返回默认值）
    // 实际实现需要调用ffprobe并解析输出
    return QTime(0, 3, 30); // 默认3分30秒
}

void AudioConverter::onConversionFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString inputFile = conversionQueue[currentConversionIndex];
    
    // 更新状态
    for (int row = 0; row < fileTable->rowCount(); ++row) {
        if (fileTable->item(row, 0)->data(Qt::UserRole).toString() == inputFile) {
            if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
                fileTable->item(row, 2)->setText("已完成");
            } else {
                fileTable->item(row, 2)->setText("转换失败");
            }
            break;
        }
    }
    
    // 继续下一个转换
    startNextConversion();
}

void AudioConverter::updateProgress()
{
    // 更新进度（可以通过解析FFmpeg输出实现更精确的进度）
}

void AudioConverter::closeEvent(QCloseEvent *event)
{
    // 如果正在转换，询问用户是否要停止
    if (ffmpegProcess && ffmpegProcess->state() == QProcess::Running) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "确认关闭",
            "音频转换正在进行中，是否要停止转换并关闭窗口？",
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (reply == QMessageBox::Yes) {
            ffmpegProcess->kill();
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        event->accept();
    }
}
