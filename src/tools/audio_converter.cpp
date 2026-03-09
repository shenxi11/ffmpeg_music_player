#include "audio_converter.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMimeData>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>

AudioConverter::AudioConverter(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle(QStringLiteral("音频转换工具"));
    setWindowIcon(QIcon(QStringLiteral(":/icon/Music.png")));

    if (!parent) {
        setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint);
    }

    setMinimumSize(900, 680);
    resize(960, 720);
    setAcceptDrops(true);

    ffmpegPath = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
    if (ffmpegPath.isEmpty()) {
        ffmpegPath = QStringLiteral("E:/ffmpeg4.4/bin/ffmpeg.exe");
    }

    outputDirectory = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    if (outputDirectory.isEmpty()) {
        outputDirectory = QDir::homePath();
    }

    ffmpegProcess = new QProcess(this);
    connect(ffmpegProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            &AudioConverter::onConversionFinished);

    setupUI();
}

void AudioConverter::setPluginHostContext(QObject* hostContext, const QStringList& grantedPermissions)
{
    m_hostContext = hostContext;
    m_grantedPermissions = grantedPermissions;

    if (statusLabel) {
        statusLabel->setText(QStringLiteral("就绪 · 已授权 %1 项权限").arg(m_grantedPermissions.size()));
    }
}

void AudioConverter::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(14);

    auto* headerCard = new QWidget(this);
    headerCard->setProperty("card", true);
    auto* headerLayout = new QVBoxLayout(headerCard);
    headerLayout->setContentsMargins(18, 14, 18, 14);
    headerLayout->setSpacing(6);

    auto* titleLabel = new QLabel(QStringLiteral("音频格式转换"), headerCard);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    auto* subtitleLabel = new QLabel(
        QStringLiteral("支持 MP3 / WAV / FLAC / AAC / OGG，支持拖拽添加与批量转换。"),
        headerCard);
    subtitleLabel->setProperty("secondary", true);

    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(subtitleLabel);

    auto* actionLayout = new QHBoxLayout();
    actionLayout->setSpacing(10);

    addAudioBtn = new QPushButton(QStringLiteral("添加音频"), this);
    importCDBtn = new QPushButton(QStringLiteral("导入CD"), this);
    clearCompletedBtn = new QPushButton(QStringLiteral("清除已完成"), this);
    clearCompletedBtn->setProperty("danger", true);

    actionLayout->addWidget(addAudioBtn);
    actionLayout->addWidget(importCDBtn);
    actionLayout->addWidget(clearCompletedBtn);
    actionLayout->addStretch();

    setupTable();

    auto* configCard = new QWidget(this);
    configCard->setProperty("card", true);
    auto* configLayout = new QVBoxLayout(configCard);
    configLayout->setContentsMargins(14, 12, 14, 12);
    configLayout->setSpacing(10);

    auto* codecRow = new QHBoxLayout();
    codecRow->setSpacing(10);

    auto* formatLabel = new QLabel(QStringLiteral("输出格式"), configCard);
    formatCombo = new QComboBox(configCard);
    formatCombo->addItems({QStringLiteral("MP3"), QStringLiteral("WAV"), QStringLiteral("FLAC"), QStringLiteral("AAC"), QStringLiteral("OGG")});

    auto* encoderLabel = new QLabel(QStringLiteral("编码策略"), configCard);
    encoderCombo = new QComboBox(configCard);

    auto* bitrateLabel = new QLabel(QStringLiteral("码率(kbps)"), configCard);
    bitrateCombo = new QComboBox(configCard);
    bitrateCombo->addItems({QStringLiteral("128"), QStringLiteral("192"), QStringLiteral("256"), QStringLiteral("320")});
    bitrateCombo->setCurrentText(QStringLiteral("320"));

    codecRow->addWidget(formatLabel);
    codecRow->addWidget(formatCombo, 1);
    codecRow->addWidget(encoderLabel);
    codecRow->addWidget(encoderCombo, 1);
    codecRow->addWidget(bitrateLabel);
    codecRow->addWidget(bitrateCombo);

    auto* outputRow = new QHBoxLayout();
    outputRow->setSpacing(10);

    auto* outputLabel = new QLabel(QStringLiteral("输出目录"), configCard);
    outputDirEdit = new QLineEdit(outputDirectory, configCard);
    outputDirEdit->setReadOnly(true);

    changeDirBtn = new QPushButton(QStringLiteral("更改目录"), configCard);
    openFolderBtn = new QPushButton(QStringLiteral("打开目录"), configCard);

    outputRow->addWidget(outputLabel);
    outputRow->addWidget(outputDirEdit, 1);
    outputRow->addWidget(changeDirBtn);
    outputRow->addWidget(openFolderBtn);

    configLayout->addLayout(codecRow);
    configLayout->addLayout(outputRow);

    auto* footerLayout = new QHBoxLayout();
    footerLayout->setSpacing(10);

    startConversionBtn = new QPushButton(QStringLiteral("开始转换"), this);
    startConversionBtn->setProperty("accent", true);

    progressBar = new QProgressBar(this);
    progressBar->setVisible(false);

    statusLabel = new QLabel(QStringLiteral("就绪"), this);
    statusLabel->setProperty("secondary", true);

    footerLayout->addWidget(startConversionBtn);
    footerLayout->addWidget(progressBar, 1);
    footerLayout->addWidget(statusLabel);

    mainLayout->addWidget(headerCard);
    mainLayout->addLayout(actionLayout);
    mainLayout->addWidget(fileTable, 1);
    mainLayout->addWidget(configCard);
    mainLayout->addLayout(footerLayout);

    connect(addAudioBtn, &QPushButton::clicked, this, &AudioConverter::onAddAudioClicked);
    connect(importCDBtn, &QPushButton::clicked, this, &AudioConverter::onImportCDClicked);
    connect(clearCompletedBtn, &QPushButton::clicked, this, &AudioConverter::onClearCompletedClicked);
    connect(changeDirBtn, &QPushButton::clicked, this, &AudioConverter::onChangeDirClicked);
    connect(openFolderBtn, &QPushButton::clicked, this, &AudioConverter::onOpenFolderClicked);
    connect(startConversionBtn, &QPushButton::clicked, this, &AudioConverter::onStartConversionClicked);
    connect(formatCombo, &QComboBox::currentTextChanged, this, &AudioConverter::onFormatChanged);
    connect(encoderCombo, &QComboBox::currentTextChanged, this, &AudioConverter::onEncoderChanged);
    connect(bitrateCombo, &QComboBox::currentTextChanged, this, &AudioConverter::onBitrateChanged);

    onFormatChanged();
}

void AudioConverter::setupTable()
{
    fileTable = new QTableWidget(0, 3, this);
    fileTable->setHorizontalHeaderLabels({QStringLiteral("文件名"), QStringLiteral("时长"), QStringLiteral("状态")});
    fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    fileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    fileTable->setAlternatingRowColors(true);
    fileTable->horizontalHeader()->setStretchLastSection(true);
    fileTable->setColumnWidth(0, 420);
    fileTable->setColumnWidth(1, 90);
}

QString AudioConverter::createButtonStyle()
{
    return QString();
}

QString AudioConverter::createComboStyle()
{
    return QString();
}

QString AudioConverter::createTableStyle()
{
    return QString();
}

void AudioConverter::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData() || !event->mimeData()->hasUrls()) {
        event->ignore();
        return;
    }

    static const QStringList kSupported = {
        QStringLiteral("mp3"), QStringLiteral("wav"), QStringLiteral("flac"), QStringLiteral("aac"),
        QStringLiteral("ogg"), QStringLiteral("m4a"), QStringLiteral("wma")
    };

    const QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl& url : urls) {
        const QFileInfo info(url.toLocalFile());
        if (kSupported.contains(info.suffix().toLower())) {
            event->acceptProposedAction();
            return;
        }
    }

    event->ignore();
}

void AudioConverter::dropEvent(QDropEvent *event)
{
    static const QStringList kSupported = {
        QStringLiteral("mp3"), QStringLiteral("wav"), QStringLiteral("flac"), QStringLiteral("aac"),
        QStringLiteral("ogg"), QStringLiteral("m4a"), QStringLiteral("wma")
    };

    const QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl& url : urls) {
        const QString filePath = url.toLocalFile();
        const QFileInfo info(filePath);
        if (kSupported.contains(info.suffix().toLower())) {
            addAudioFile(filePath);
        }
    }

    event->acceptProposedAction();
}

void AudioConverter::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
}

void AudioConverter::onAddAudioClicked()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this,
        QStringLiteral("选择音频文件"),
        QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
        QStringLiteral("音频文件 (*.mp3 *.wav *.flac *.aac *.ogg *.m4a *.wma)"));

    for (const QString& file : files) {
        addAudioFile(file);
    }
}

void AudioConverter::onImportCDClicked()
{
    QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("CD 导入功能暂未实现。"));
}

void AudioConverter::onClearCompletedClicked()
{
    for (int row = fileTable->rowCount() - 1; row >= 0; --row) {
        QTableWidgetItem* statusItem = fileTable->item(row, 2);
        if (!statusItem) {
            continue;
        }
        if (statusItem->text() == QStringLiteral("已完成")) {
            fileTable->removeRow(row);
        }
    }
}

void AudioConverter::onChangeDirClicked()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("选择输出目录"),
        outputDirectory);

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
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先添加音频文件。"));
        return;
    }

    if (ffmpegPath.isEmpty() || !QFile::exists(ffmpegPath)) {
        QMessageBox::critical(this,
                              QStringLiteral("错误"),
                              QStringLiteral("未找到 FFmpeg 程序，请检查 FFmpeg 安装或路径配置。\n当前路径：%1").arg(ffmpegPath));
        return;
    }

    conversionQueue.clear();
    for (int row = 0; row < fileTable->rowCount(); ++row) {
        QTableWidgetItem* pathItem = fileTable->item(row, 0);
        QTableWidgetItem* statusItem = fileTable->item(row, 2);
        if (!pathItem || !statusItem) {
            continue;
        }
        if (statusItem->text() != QStringLiteral("已完成")) {
            conversionQueue.append(pathItem->data(Qt::UserRole).toString());
            statusItem->setText(QStringLiteral("等待中"));
        }
    }

    if (conversionQueue.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("没有需要转换的文件。"));
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
    const QString format = formatCombo->currentText().toUpper();

    encoderCombo->clear();
    if (format == QStringLiteral("MP3")) {
        encoderCombo->addItems({QStringLiteral("VBR V0"), QStringLiteral("VBR V2"), QStringLiteral("CBR")});
        encoderCombo->setCurrentText(QStringLiteral("VBR V0"));
    } else if (format == QStringLiteral("AAC")) {
        encoderCombo->addItems({QStringLiteral("AAC-LC"), QStringLiteral("HE-AAC")});
        encoderCombo->setCurrentText(QStringLiteral("AAC-LC"));
    } else if (format == QStringLiteral("OGG")) {
        encoderCombo->addItems({QStringLiteral("Vorbis")});
    } else if (format == QStringLiteral("FLAC")) {
        encoderCombo->addItems({QStringLiteral("FLAC")});
    } else {
        encoderCombo->addItems({QStringLiteral("PCM")});
    }

    onEncoderChanged();
}

void AudioConverter::onEncoderChanged()
{
    const QString format = formatCombo->currentText().toUpper();
    const QString encoder = encoderCombo->currentText();

    bool enableBitrate = false;
    if (format == QStringLiteral("MP3")) {
        enableBitrate = (encoder == QStringLiteral("CBR"));
    } else if (format == QStringLiteral("AAC")) {
        enableBitrate = true;
    }

    bitrateCombo->setEnabled(enableBitrate);
}

void AudioConverter::onBitrateChanged()
{
}

void AudioConverter::addAudioFile(const QString &filePath)
{
    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        return;
    }

    for (int row = 0; row < fileTable->rowCount(); ++row) {
        QTableWidgetItem* item = fileTable->item(row, 0);
        if (item && item->data(Qt::UserRole).toString() == filePath) {
            return;
        }
    }

    const int row = fileTable->rowCount();
    fileTable->insertRow(row);

    auto* nameItem = new QTableWidgetItem(fileInfo.fileName());
    nameItem->setData(Qt::UserRole, filePath);
    fileTable->setItem(row, 0, nameItem);

    const QTime duration = getAudioDuration(filePath);
    fileTable->setItem(row, 1, new QTableWidgetItem(duration.toString(QStringLiteral("mm:ss"))));
    fileTable->setItem(row, 2, new QTableWidgetItem(QStringLiteral("等待转换")));
}

void AudioConverter::startNextConversion()
{
    ++currentConversionIndex;

    if (currentConversionIndex >= conversionQueue.size()) {
        progressBar->setVisible(false);
        startConversionBtn->setEnabled(true);
        statusLabel->setText(QStringLiteral("转换完成"));
        QMessageBox::information(this, QStringLiteral("完成"), QStringLiteral("全部文件转换完成。"));
        return;
    }

    const QString inputFile = conversionQueue.at(currentConversionIndex);
    const QString outputFile = getOutputPath(inputFile);

    for (int row = 0; row < fileTable->rowCount(); ++row) {
        QTableWidgetItem* pathItem = fileTable->item(row, 0);
        if (pathItem && pathItem->data(Qt::UserRole).toString() == inputFile) {
            QTableWidgetItem* statusItem = fileTable->item(row, 2);
            if (statusItem) {
                statusItem->setText(QStringLiteral("转换中"));
            }
            break;
        }
    }

    progressBar->setValue(currentConversionIndex);
    statusLabel->setText(QStringLiteral("正在转换: %1").arg(QFileInfo(inputFile).fileName()));

    QStringList arguments;
    arguments << QStringLiteral("-i") << inputFile;

    const QString format = formatCombo->currentText().toLower();
    const QString encoder = encoderCombo->currentText();

    if (format == QStringLiteral("mp3")) {
        arguments << QStringLiteral("-acodec") << QStringLiteral("libmp3lame");
        if (encoder == QStringLiteral("VBR V0")) {
            arguments << QStringLiteral("-q:a") << QStringLiteral("0");
        } else if (encoder == QStringLiteral("VBR V2")) {
            arguments << QStringLiteral("-q:a") << QStringLiteral("2");
        } else {
            arguments << QStringLiteral("-b:a") << (bitrateCombo->currentText() + QStringLiteral("k"));
        }
    } else if (format == QStringLiteral("aac")) {
        arguments << QStringLiteral("-acodec") << QStringLiteral("aac");
        arguments << QStringLiteral("-b:a") << (bitrateCombo->currentText() + QStringLiteral("k"));
    } else if (format == QStringLiteral("wav")) {
        arguments << QStringLiteral("-acodec") << QStringLiteral("pcm_s16le");
    } else if (format == QStringLiteral("flac")) {
        arguments << QStringLiteral("-acodec") << QStringLiteral("flac");
        arguments << QStringLiteral("-compression_level") << QStringLiteral("5");
    } else if (format == QStringLiteral("ogg")) {
        arguments << QStringLiteral("-acodec") << QStringLiteral("libvorbis");
        arguments << QStringLiteral("-q:a") << QStringLiteral("5");
    }

    arguments << QStringLiteral("-vn");
    arguments << QStringLiteral("-y");
    arguments << outputFile;

    ffmpegProcess->start(ffmpegPath, arguments);
}

QString AudioConverter::getOutputPath(const QString &inputPath)
{
    const QFileInfo inputInfo(inputPath);
    const QString format = formatCombo->currentText().toLower();
    const QString outputFileName = inputInfo.baseName() + QStringLiteral(".") + format;
    return QDir(outputDirectory).filePath(outputFileName);
}

QTime AudioConverter::getAudioDuration(const QString &filePath)
{
    Q_UNUSED(filePath);
    return QTime(0, 3, 30);
}

void AudioConverter::onConversionFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (currentConversionIndex < 0 || currentConversionIndex >= conversionQueue.size()) {
        startNextConversion();
        return;
    }

    const QString inputFile = conversionQueue.at(currentConversionIndex);

    for (int row = 0; row < fileTable->rowCount(); ++row) {
        QTableWidgetItem* pathItem = fileTable->item(row, 0);
        if (pathItem && pathItem->data(Qt::UserRole).toString() == inputFile) {
            QTableWidgetItem* statusItem = fileTable->item(row, 2);
            if (!statusItem) {
                break;
            }
            if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
                statusItem->setText(QStringLiteral("已完成"));
            } else {
                statusItem->setText(QStringLiteral("转换失败"));
            }
            break;
        }
    }

    startNextConversion();
}

void AudioConverter::updateProgress()
{
}

void AudioConverter::closeEvent(QCloseEvent *event)
{
    if (ffmpegProcess && ffmpegProcess->state() == QProcess::Running) {
        const QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            QStringLiteral("确认关闭"),
            QStringLiteral("当前仍在转换，是否停止转换并关闭窗口？"),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            ffmpegProcess->kill();
            event->accept();
        } else {
            event->ignore();
        }
        return;
    }

    event->accept();
}
