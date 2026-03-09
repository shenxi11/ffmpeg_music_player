#include "translate_widget.h"

#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QMetaObject>
#include <QTextStream>
#include <QThread>
#include <QUrl>
#include <QVBoxLayout>
#include <chrono>

transformFactory::transformFactory()
{
    funcMp[QStringLiteral("txt")] = saveTXT;
    funcMp[QStringLiteral("json")] = saveJSON;
    funcMp[QStringLiteral("vtt")] = saveVTT;
    funcMp[QStringLiteral("srt")] = saveSRT;
    funcMp[QStringLiteral("lrc")] = saveLRC;
    funcMp[QStringLiteral("kar")] = saveKAR;
}

void transformFactory::save(const QString &name, struct whisper_context* ctx, QStringList &outputLines)
{
    const auto it = funcMp.find(name.toLower());
    if (it != funcMp.end()) {
        it->second(ctx, outputLines);
    }
}

ResultListItem::ResultListItem(const QString &fileName, const QString &filePath, QWidget *parent)
    : QWidget(parent)
    , fileName(fileName)
    , filePath(filePath)
{
    setProperty("card", true);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 10, 12, 10);
    mainLayout->setSpacing(6);

    fileNameLabel = new QLabel(fileName, this);
    QFont nameFont = fileNameLabel->font();
    nameFont.setBold(true);
    fileNameLabel->setFont(nameFont);

    QFileInfo fileInfo(filePath);
    filePathLabel = new QLabel(fileInfo.absolutePath(), this);
    filePathLabel->setProperty("secondary", true);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(8);
    buttonLayout->addStretch();

    openFileBtn = new QPushButton(QStringLiteral("打开文件"), this);
    openFolderBtn = new QPushButton(QStringLiteral("打开目录"), this);

    buttonLayout->addWidget(openFileBtn);
    buttonLayout->addWidget(openFolderBtn);

    mainLayout->addWidget(fileNameLabel);
    mainLayout->addWidget(filePathLabel);
    mainLayout->addLayout(buttonLayout);

    connect(openFileBtn, &QPushButton::clicked, this, &ResultListItem::onOpenFileClicked);
    connect(openFolderBtn, &QPushButton::clicked, this, &ResultListItem::onOpenFolderClicked);
}

void ResultListItem::onOpenFileClicked()
{
    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("文件不存在：%1").arg(filePath));
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
}

void ResultListItem::onOpenFolderClicked()
{
    const QFileInfo fileInfo(filePath);
    const QString folderPath = fileInfo.absolutePath();
    if (!QFileInfo(folderPath).exists()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("目录不存在：%1").arg(folderPath));
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
}

TranslateWidget::TranslateWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle(QStringLiteral("Whisper 音频转写"));

    if (!parent) {
        setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint);
    }

    setMinimumSize(760, 640);
    resize(820, 720);

    decodeThread_ = new QThread(this);
    take_pcm = std::make_shared<TakePcm>();
    take_pcm->setTranslate(true);
    take_pcm->moveToThread(decodeThread_);
    decodeThread_->start();

    buildUi();
    bindSignals();

    config_.modelName_ = QStringLiteral("ggml-tiny.bin");
    config_.outputMode_ = QStringLiteral("txt");
    config_.language_ = 0;
}

TranslateWidget::~TranslateWidget()
{
    if (decodeThread_) {
        decodeThread_->quit();
        decodeThread_->wait(1500);
    }
}

void TranslateWidget::setPluginHostContext(QObject* hostContext, const QStringList& grantedPermissions)
{
    m_hostContext = hostContext;
    m_grantedPermissions = grantedPermissions;
    if (subtitleLabel) {
        subtitleLabel->setText(QStringLiteral("Whisper.cpp 本地转写 · 已授权 %1 项插件权限").arg(m_grantedPermissions.size()));
    }
}

void TranslateWidget::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 20);
    root->setSpacing(14);

    auto* headerCard = new QWidget(this);
    headerCard->setProperty("card", true);
    auto* headerLayout = new QVBoxLayout(headerCard);
    headerLayout->setContentsMargins(18, 14, 18, 14);
    headerLayout->setSpacing(6);

    auto* titleLabel = new QLabel(QStringLiteral("音频转文字"), headerCard);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    subtitleLabel = new QLabel(QStringLiteral("Whisper.cpp 本地转写"), headerCard);
    subtitleLabel->setProperty("secondary", true);

    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(subtitleLabel);

    auto* configCard = new QWidget(this);
    configCard->setProperty("card", true);
    auto* configLayout = new QVBoxLayout(configCard);
    configLayout->setContentsMargins(14, 12, 14, 12);
    configLayout->setSpacing(10);

    auto* fileLayout = new QHBoxLayout();
    fileLabel = new QLabel(QStringLiteral("音频文件"), configCard);
    filePathEdit = new QLineEdit(configCard);
    filePathEdit->setPlaceholderText(QStringLiteral("请选择待转写音频文件"));
    browseButton = new QPushButton(QStringLiteral("浏览"), configCard);
    fileLayout->addWidget(fileLabel);
    fileLayout->addWidget(filePathEdit, 1);
    fileLayout->addWidget(browseButton);

    auto* optionLayout = new QHBoxLayout();

    modelLabel = new QLabel(QStringLiteral("模型"), configCard);
    modelCombo = new QComboBox(configCard);
    modelCombo->addItems({QStringLiteral("tiny"), QStringLiteral("base"), QStringLiteral("small"), QStringLiteral("medium"), QStringLiteral("large")});

    formatLabel = new QLabel(QStringLiteral("输出格式"), configCard);
    formatCombo = new QComboBox(configCard);
    formatCombo->addItems({QStringLiteral("txt"), QStringLiteral("srt"), QStringLiteral("vtt"), QStringLiteral("json"), QStringLiteral("lrc"), QStringLiteral("kar")});

    langLabel = new QLabel(QStringLiteral("音频语言"), configCard);
    langCombo = new QComboBox(configCard);
    langCombo->addItems({
        QStringLiteral("中文"),
        QStringLiteral("英文"),
        QStringLiteral("日语"),
        QStringLiteral("韩语"),
        QStringLiteral("法语"),
        QStringLiteral("德语"),
        QStringLiteral("西班牙语")
    });

    optionLayout->addWidget(modelLabel);
    optionLayout->addWidget(modelCombo);
    optionLayout->addWidget(formatLabel);
    optionLayout->addWidget(formatCombo);
    optionLayout->addWidget(langLabel);
    optionLayout->addWidget(langCombo);

    transcribeButton = new QPushButton(QStringLiteral("开始转写"), configCard);
    transcribeButton->setProperty("accent", true);

    progressBar = new QProgressBar(configCard);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);

    configLayout->addLayout(fileLayout);
    configLayout->addLayout(optionLayout);
    configLayout->addWidget(transcribeButton);
    configLayout->addWidget(progressBar);

    auto* outputCard = new QWidget(this);
    outputCard->setProperty("card", true);
    auto* outputLayout = new QVBoxLayout(outputCard);
    outputLayout->setContentsMargins(14, 12, 14, 12);
    outputLayout->setSpacing(8);

    auto* previewLabel = new QLabel(QStringLiteral("转写预览"), outputCard);
    previewLabel->setProperty("secondary", true);
    resultEdit = new QTextEdit(outputCard);
    resultEdit->setReadOnly(true);
    resultEdit->setPlaceholderText(QStringLiteral("转写结果预览将在这里显示"));

    resultListLabel = new QLabel(QStringLiteral("转写输出记录"), outputCard);
    resultListLabel->setProperty("secondary", true);

    resultList = new QListWidget(outputCard);
    resultList->setMinimumHeight(180);

    outputLayout->addWidget(previewLabel);
    outputLayout->addWidget(resultEdit);
    outputLayout->addWidget(resultListLabel);
    outputLayout->addWidget(resultList);

    root->addWidget(headerCard);
    root->addWidget(configCard);
    root->addWidget(outputCard, 1);

    mainLayout = root;
}

void TranslateWidget::bindSignals()
{
    connect(modelCombo, &QComboBox::currentTextChanged, this, [this]() {
        config_.modelName_ = QStringLiteral("ggml-") + modelCombo->currentText() + QStringLiteral(".bin");
    });

    connect(formatCombo, &QComboBox::currentTextChanged, this, [this]() {
        config_.outputMode_ = formatCombo->currentText();
    });

    connect(langCombo, &QComboBox::currentTextChanged, this, [this]() {
        config_.language_ = langCombo->currentIndex();
    });

    connect(browseButton, &QPushButton::clicked, this, [this]() {
        const QString fileName = QFileDialog::getOpenFileName(
            this,
            QStringLiteral("选择音频文件"),
            QString(),
            QStringLiteral("音频文件 (*.wav *.mp3 *.aac *.flac *.m4a *.ogg)"));
        if (!fileName.isEmpty()) {
            config_.audioPath_ = fileName;
            filePathEdit->setText(fileName);
        }
    });

    connect(transcribeButton, &QPushButton::clicked, this, &TranslateWidget::on_transcribeButton_clicked);

    connect(this, &TranslateWidget::signal_begin_tranform, this, &TranslateWidget::on_signal_begin_transform);
    connect(this, &TranslateWidget::signal_erorr, this, &TranslateWidget::showTipMessage);

    connect(this, &TranslateWidget::signal_begin_take_pcm, take_pcm.get(), &TakePcm::signal_begin_make_pcm);
    connect(take_pcm.get(), &TakePcm::signal_send_data, this, &TranslateWidget::on_signal_send_data);
    connect(take_pcm.get(), &TakePcm::signal_decodeEnd, this, &TranslateWidget::on_signal_decodeEnd);
    connect(this, &TranslateWidget::signal_outFile, this, &TranslateWidget::on_signal_outFile);
}

void TranslateWidget::on_signal_decodeEnd()
{
    {
        std::lock_guard<std::mutex> lock(mtx);
        pcmReady.store(true);
    }
    cv.notify_one();
}

void TranslateWidget::on_signal_send_data(uint8_t *buffer, int bufferSize, qint64 timeMap)
{
    Q_UNUSED(timeMap);

    int16_t* samples = reinterpret_cast<int16_t*>(buffer);
    const int sampleCount = bufferSize / sizeof(int16_t) / 2;

    pcmf32_.reserve(pcmf32_.size() + sampleCount);
    for (int i = 0; i < sampleCount; ++i) {
        const float left = samples[i * 2] / 32768.0f;
        const float right = samples[i * 2 + 1] / 32768.0f;
        pcmf32_.append((left + right) * 0.5f);
    }

    free(buffer);
}

void TranslateWidget::updateProgress(int progress)
{
    progressBar->setValue(progress);
}

void TranslateWidget::on_signal_outFile(const QStringList &segments)
{
    resultEdit->clear();
    for (const QString& line : segments) {
        resultEdit->append(line);
    }

    QFileDialog dialog(this);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setWindowTitle(QStringLiteral("保存转写文件"));

    const QFileInfo inputInfo(config_.audioPath_);
    dialog.setDirectory(inputInfo.absolutePath());

    const QString suffix = config_.outputMode_.toLower();
    dialog.setDefaultSuffix(suffix);
    dialog.setNameFilter(QStringLiteral("%1 files (*.%1)").arg(suffix));

    if (dialog.exec()) {
        const QString fileName = dialog.selectedFiles().first();
        QFile outFile(fileName);
        if (outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&outFile);
            for (const QString& segment : segments) {
                out << segment << '\n';
            }
            outFile.close();

            QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("转写结果已保存。"));

            QFileInfo fileInfo(fileName);
            addResultItem(fileInfo.fileName(), fileName);
        } else {
            QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("无法写入文件：%1").arg(fileName));
        }
    }

    resetUiStateAfterTask();
}

void TranslateWidget::showTipMessage(const QString &msg)
{
    QMessageBox::warning(this, QStringLiteral("提示"), msg);
    resetUiStateAfterTask();
}

void TranslateWidget::resetUiStateAfterTask()
{
    translating.store(false);
    transcribeButton->setText(QStringLiteral("开始转写"));
    transcribeButton->setEnabled(true);
    if (progressBar->value() < progressBar->maximum()) {
        progressBar->setValue(0);
    }
}

void TranslateWidget::on_signal_begin_transform()
{
    QThread* worker = QThread::create([this]() {
        struct whisper_context* ctx = whisper_init_from_file((modelPath + config_.modelName_).toStdString().c_str());
        if (!ctx) {
            emit signal_erorr(QStringLiteral("模型加载失败，请检查模型路径。"));
            return;
        }

        whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
        params.print_progress = false;
        params.detect_language = false;

        const QString lang = languageMap.value(config_.language_, QStringLiteral("zh"));
        const std::string langStd = lang.toStdString();
        params.language = langStd.c_str();
        params.initial_prompt = initialPromptMap.value(lang, "");
        params.temperature = 0.1f;

        params.progress_callback = [](struct whisper_context*, struct whisper_state*, int progress, void* user_data) {
            QMetaObject::invokeMethod(static_cast<QObject*>(user_data),
                                      "updateProgress",
                                      Qt::QueuedConnection,
                                      Q_ARG(int, progress));
        };
        params.progress_callback_user_data = this;

        emit signal_begin_take_pcm(config_.audioPath_);

        {
            std::unique_lock<std::mutex> lock(mtx);
            const bool ready = cv.wait_for(lock, std::chrono::seconds(30), [this]() { return pcmReady.load(); });
            if (!ready) {
                whisper_free(ctx);
                emit signal_erorr(QStringLiteral("音频解码超时，请重试。"));
                return;
            }
        }

        if (pcmf32_.isEmpty()) {
            whisper_free(ctx);
            emit signal_erorr(QStringLiteral("未读取到有效音频数据。"));
            return;
        }

        const int ret = whisper_full(ctx, params, pcmf32_.data(), pcmf32_.size());
        if (ret != 0) {
            whisper_free(ctx);
            emit signal_erorr(QStringLiteral("语音转写失败。"));
            return;
        }

        QStringList outputLines;
        factory.save(config_.outputMode_, ctx, outputLines);

        whisper_free(ctx);
        emit signal_outFile(outputLines);
    });

    connect(worker, &QThread::finished, worker, &QThread::deleteLater);
    worker->start();
}

void TranslateWidget::on_transcribeButton_clicked()
{
    if (translating.load()) {
        return;
    }

    config_.audioPath_ = filePathEdit->text().trimmed();
    if (config_.audioPath_.isEmpty() || !QFileInfo(config_.audioPath_).exists()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择有效的音频文件。"));
        return;
    }

    translating.store(true);
    pcmReady.store(false);
    pcmf32_.clear();

    transcribeButton->setText(QStringLiteral("转写中..."));
    transcribeButton->setEnabled(false);
    progressBar->setValue(0);

    emit signal_begin_tranform();
}

void TranslateWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
}

void saveTXT(struct whisper_context* ctx, QStringList &outputLines)
{
    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        outputLines << QString::fromUtf8(whisper_full_get_segment_text(ctx, i));
    }
}

void saveJSON(struct whisper_context* ctx, QStringList &outputLines)
{
    QJsonArray segments;
    const int n_segments = whisper_full_n_segments(ctx);

    for (int i = 0; i < n_segments; ++i) {
        QJsonObject obj;
        obj[QStringLiteral("start")] = static_cast<double>(whisper_full_get_segment_t0(ctx, i) * 10);
        obj[QStringLiteral("end")] = static_cast<double>(whisper_full_get_segment_t1(ctx, i) * 10);
        obj[QStringLiteral("text")] = QString::fromUtf8(whisper_full_get_segment_text(ctx, i)).trimmed();
        segments.append(obj);
    }

    QJsonObject root;
    root[QStringLiteral("lyrics")] = segments;
    outputLines << QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

void saveKAR(struct whisper_context* ctx, QStringList &outputLines)
{
    outputLines << QStringLiteral("@TITLE Generated Lyrics");
    outputLines << QStringLiteral("@V0100");
    outputLines << QStringLiteral("@KMIDI KARAOKE FILE");
    outputLines << QString();

    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const int64_t startMs = whisper_full_get_segment_t0(ctx, i) * 10;
        const int minutes = static_cast<int>(startMs / 60000);
        const int seconds = static_cast<int>((startMs % 60000) / 1000);
        const int centiseconds = static_cast<int>((startMs % 1000) / 10);

        const QString timeTag = QStringLiteral("%1:%2:%3")
                                    .arg(minutes, 2, 10, QChar('0'))
                                    .arg(seconds, 2, 10, QChar('0'))
                                    .arg(centiseconds, 2, 10, QChar('0'));

        const QString text = QString::fromUtf8(whisper_full_get_segment_text(ctx, i)).trimmed();
        outputLines << QStringLiteral("%1 [0000][0000][0000] %2").arg(timeTag, text);
    }
}

void saveLRC(struct whisper_context* ctx, QStringList &outputLines)
{
    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const int64_t startMs = whisper_full_get_segment_t0(ctx, i) * 10;
        const int minutes = static_cast<int>(startMs / 60000);
        const int seconds = static_cast<int>((startMs % 60000) / 1000);
        const int centiseconds = static_cast<int>((startMs % 1000) / 10);

        const QString timeTag = QStringLiteral("[%1:%2.%3]")
                                    .arg(minutes, 2, 10, QChar('0'))
                                    .arg(seconds, 2, 10, QChar('0'))
                                    .arg(centiseconds, 2, 10, QChar('0'));

        const QString text = QString::fromUtf8(whisper_full_get_segment_text(ctx, i)).trimmed();
        outputLines << (timeTag + text);
    }
}

void saveVTT(struct whisper_context* ctx, QStringList &outputLines)
{
    outputLines << QStringLiteral("WEBVTT");
    outputLines << QString();

    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const int64_t startMs = whisper_full_get_segment_t0(ctx, i) * 10;
        const int64_t endMs = whisper_full_get_segment_t1(ctx, i) * 10;

        const QString startStr = QStringLiteral("%1:%2:%3.%4")
                                     .arg(startMs / 3600000, 2, 10, QChar('0'))
                                     .arg((startMs % 3600000) / 60000, 2, 10, QChar('0'))
                                     .arg((startMs % 60000) / 1000, 2, 10, QChar('0'))
                                     .arg(startMs % 1000, 3, 10, QChar('0'));

        const QString endStr = QStringLiteral("%1:%2:%3.%4")
                                   .arg(endMs / 3600000, 2, 10, QChar('0'))
                                   .arg((endMs % 3600000) / 60000, 2, 10, QChar('0'))
                                   .arg((endMs % 60000) / 1000, 2, 10, QChar('0'))
                                   .arg(endMs % 1000, 3, 10, QChar('0'));

        outputLines << QString::number(i + 1);
        outputLines << (startStr + QStringLiteral(" --> ") + endStr);
        outputLines << QString::fromUtf8(whisper_full_get_segment_text(ctx, i)).trimmed();
        outputLines << QString();
    }
}

void saveSRT(struct whisper_context* ctx, QStringList &outputLines)
{
    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const int64_t startMs = whisper_full_get_segment_t0(ctx, i) * 10;
        const int64_t endMs = whisper_full_get_segment_t1(ctx, i) * 10;

        const QString startStr = QStringLiteral("%1:%2:%3,%4")
                                     .arg(startMs / 3600000, 2, 10, QChar('0'))
                                     .arg((startMs % 3600000) / 60000, 2, 10, QChar('0'))
                                     .arg((startMs % 60000) / 1000, 2, 10, QChar('0'))
                                     .arg(startMs % 1000, 3, 10, QChar('0'));

        const QString endStr = QStringLiteral("%1:%2:%3,%4")
                                   .arg(endMs / 3600000, 2, 10, QChar('0'))
                                   .arg((endMs % 3600000) / 60000, 2, 10, QChar('0'))
                                   .arg((endMs % 60000) / 1000, 2, 10, QChar('0'))
                                   .arg(endMs % 1000, 3, 10, QChar('0'));

        outputLines << QString::number(i + 1);
        outputLines << (startStr + QStringLiteral(" --> ") + endStr);
        outputLines << QString::fromUtf8(whisper_full_get_segment_text(ctx, i)).trimmed();
        outputLines << QString();
    }
}

void TranslateWidget::addResultItem(const QString &fileName, const QString &filePath)
{
    auto* itemWidget = new ResultListItem(fileName, filePath, this);
    auto* listItem = new QListWidgetItem(resultList);
    listItem->setSizeHint(itemWidget->sizeHint());
    resultList->setItemWidget(listItem, itemWidget);
    resultList->scrollToBottom();
}
