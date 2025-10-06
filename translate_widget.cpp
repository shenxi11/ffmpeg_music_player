
#include "translate_widget.h"
#include <QPainter>
#include <QStyleOption>
#include <QDebug>
#include <QProcess>
#include <QString>
#include <QThread>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTime>
#include <QMessageBox>

// ResultListItem 类的实现
ResultListItem::ResultListItem(const QString &fileName, const QString &filePath, QWidget *parent)
    : QWidget(parent), fileName(fileName), filePath(filePath)
{
    setFixedHeight(80);
    setStyleSheet(
        "ResultListItem {"
        "    background: #f8f9fa;"
        "    border: 1px solid #e9ecef;"
        "    border-radius: 8px;"
        "    margin: 2px;"
        "}"
        "ResultListItem:hover {"
        "    background: #e9ecef;"
        "}"
    );

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 8, 12, 8);
    mainLayout->setSpacing(4);

    // 文件名标签
    fileNameLabel = new QLabel(fileName, this);
    fileNameLabel->setStyleSheet("font-weight: bold; color: #495057; font-size: 14px;");
    fileNameLabel->setWordWrap(true);

    // 文件路径标签
    QFileInfo fileInfo(filePath);
    QString displayPath = fileInfo.dir().absolutePath();
    filePathLabel = new QLabel(displayPath, this);
    filePathLabel->setStyleSheet("color: #6c757d; font-size: 12px;");
    filePathLabel->setWordWrap(true);

    // 按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    openFileBtn = new QPushButton("打开文件", this);
    openFileBtn->setFixedSize(80, 24);
    openFileBtn->setStyleSheet(
        "QPushButton {"
        "    background: #007bff;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 4px;"
        "    font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "    background: #0056b3;"
        "}"
        "QPushButton:pressed {"
        "    background: #004085;"
        "}"
    );

    openFolderBtn = new QPushButton("打开文件夹", this);
    openFolderBtn->setFixedSize(80, 24);
    openFolderBtn->setStyleSheet(
        "QPushButton {"
        "    background: #28a745;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 4px;"
        "    font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "    background: #1e7e34;"
        "}"
        "QPushButton:pressed {"
        "    background: #155724;"
        "}"
    );

    buttonLayout->addStretch();
    buttonLayout->addWidget(openFileBtn);
    buttonLayout->addWidget(openFolderBtn);
    
    mainLayout->addWidget(fileNameLabel);
    mainLayout->addWidget(filePathLabel);
    mainLayout->addLayout(buttonLayout);

    // 连接信号
    connect(openFileBtn, &QPushButton::clicked, this, &ResultListItem::onOpenFileClicked);
    connect(openFolderBtn, &QPushButton::clicked, this, &ResultListItem::onOpenFolderClicked);
}

void ResultListItem::onOpenFileClicked()
{
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    } else {
        QMessageBox::warning(this, "警告", "文件不存在：" + filePath);
    }
}

void ResultListItem::onOpenFolderClicked()
{
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists()) {
        QString folderPath = fileInfo.dir().absolutePath();
        QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
    } else {
        QMessageBox::warning(this, "警告", "文件夹不存在：" + fileInfo.dir().absolutePath());
    }
}

void transformFactory::save(const QString &name, struct whisper_context* ctx, QStringList &outputLines){
    auto func = funcMp.find(name);
    if(func != funcMp.end()){
        func->second(ctx, outputLines);
    }
}
TranslateWidget::TranslateWidget(QWidget *parent) : QWidget(parent)
{

    this->resize(500, 400);
    // 当作为内嵌控件时，不设置无边框窗口属性
    if (!parent) {
        this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint);
        this->setAttribute(Qt::WA_TranslucentBackground);
    }

    // 顶部标题栏和窗口控制按钮 - 只在独立窗口时显示
    if (!parent) {
        titleBarWidget = new QWidget(this);
        titleBarWidget->setFixedHeight(40);
        titleBarWidget->setStyleSheet("background: transparent;");

        titleLabel = new QLabel("音频转文字", titleBarWidget);
        titleLabel->setStyleSheet("color: #333; font-size: 20px; font-weight: bold;");
        titleLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);


        // 右上角三个按钮使用图片
        minimizeButton = new QPushButton(titleBarWidget);
        maximizeButton = new QPushButton(titleBarWidget);
        closeButton = new QPushButton(titleBarWidget);
        minimizeButton->setFixedSize(28, 28);
        maximizeButton->setFixedSize(28, 28);
        closeButton->setFixedSize(28, 28);
        minimizeButton->setToolTip("最小化");
        maximizeButton->setToolTip("最大化/还原");
        closeButton->setToolTip("关闭");
        minimizeButton->setText("");
        maximizeButton->setText("");
        closeButton->setText("");
        minimizeButton->setStyleSheet("QPushButton { border-image: url(:/new/prefix1/icon/方形未选中.png); background:transparent; border:none; } QPushButton:hover{background:#e0e0e0;}");
        maximizeButton->setStyleSheet("QPushButton { border-image: url(:/new/prefix1/icon/减号.png); background:transparent; border:none; } QPushButton:hover{background:#e0e0e0;}");
        closeButton->setStyleSheet("QPushButton { border-image: url(:/new/prefix1/icon/关闭1.png); background:transparent; border:none; } QPushButton:hover{background:#ffeaea;}");



        titleBarLayout = new QHBoxLayout(titleBarWidget);
        titleBarLayout->addWidget(titleLabel);
        titleBarLayout->addStretch();
        titleBarLayout->addWidget(maximizeButton);
        titleBarLayout->addWidget(minimizeButton);
        titleBarLayout->addWidget(closeButton);
        titleBarLayout->setContentsMargins(0, 0, 0, 0);
        titleBarLayout->setSpacing(2);
    }


    // 文件选择
    fileLabel = new QLabel("音频文件:", this);
    filePathEdit = new QLineEdit(this);
    filePathEdit->setPlaceholderText("请选择音频文件...");
    browseButton = new QPushButton("浏览", this);
    browseButton->setStyleSheet("background-color: #1DB954; color: white; border-radius: 4px; padding: 4px 12px;");
    fileLayout = new QHBoxLayout();
    fileLayout->addWidget(fileLabel);
    fileLayout->addWidget(filePathEdit);
    fileLayout->addWidget(browseButton);

    // 模型选择
    modelLabel = new QLabel("模型:", this);
    modelCombo = new QComboBox(this);
    modelCombo->addItems({"tiny", "base", "small", "medium", "large"});
    connect(modelCombo, &QComboBox::currentTextChanged, this,[=](){
        config_.modelName_ = "ggml-" + modelCombo->currentText() + ".bin";
    });
    modelLayout = new QHBoxLayout();
    modelLayout->addWidget(modelLabel);
    modelLayout->addWidget(modelCombo);

    // 输出格式选择
    formatLabel = new QLabel("输出格式:", this);
    formatCombo = new QComboBox(this);
    formatCombo->addItems({"txt", "srt", "vtt", "json", "lrc"});
    connect(formatCombo, &QComboBox::currentTextChanged, this,[=](){
        config_.outputMode_ = formatCombo->currentText();
    });
    formatLayout = new QHBoxLayout();
    formatLayout->addWidget(formatLabel);
    formatLayout->addWidget(formatCombo);

    // 音频语言选择
    langLabel = new QLabel("音频语言:", this);
    langCombo = new QComboBox(this);
    langCombo->addItems({"中文", "英文", "日语", "韩语", "法语", "德语", "西班牙语"});
    connect(langCombo, &QComboBox::currentTextChanged, this,[=](){
        config_.language_ = langCombo->currentIndex();
    });
    langLayout = new QHBoxLayout();
    langLayout->addWidget(langLabel);
    langLayout->addWidget(langCombo);

    // 转换按钮
    transcribeButton = new QPushButton("开始转换", this);
    transcribeButton->setStyleSheet("background-color: #FF4766; color: white; border-radius: 4px; font-size: 16px; padding: 6px 0;");
    connect(transcribeButton, &QPushButton::clicked, this, &TranslateWidget::on_transcribeButton_clicked);

    // 进度条
    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setTextVisible(true);
    progressBar->setStyleSheet("QProgressBar{border-radius:6px; background:#f0f0f0; border:1px solid #e0e0e0;} QProgressBar::chunk{background-color:#1DB954; border-radius:6px;}");

    // 结果显示
    resultEdit = new QTextEdit(this);
    resultEdit->setPlaceholderText("转换结果将在此显示...");
    resultEdit->setReadOnly(true);
    resultEdit->setStyleSheet("background: #f8f8f8; border-radius: 6px; border: 1px solid #e0e0e0;");

    // 结果列表
    resultListLabel = new QLabel("转换历史记录:", this);
    resultListLabel->setStyleSheet("font-weight: bold; color: #333; font-size: 14px;");
    
    resultList = new QListWidget(this);
    resultList->setMaximumHeight(150);
    resultList->setStyleSheet(
        "QListWidget {"
        "    background: #ffffff;"
        "    border: 1px solid #e0e0e0;"
        "    border-radius: 6px;"
        "    padding: 4px;"
        "}"
        "QListWidget::item {"
        "    border: none;"
        "    margin: 1px;"
        "}"
    );

    // 主布局
    mainLayout = new QVBoxLayout(this);
    if (!parent && titleBarWidget) {
        mainLayout->addWidget(titleBarWidget);
    }
    mainLayout->addLayout(fileLayout);
    mainLayout->addLayout(modelLayout);
    mainLayout->addLayout(formatLayout);
    mainLayout->addLayout(langLayout);
    mainLayout->addWidget(transcribeButton);
    mainLayout->addWidget(progressBar);
    mainLayout->addWidget(resultEdit);
    mainLayout->addWidget(resultListLabel);
    mainLayout->addWidget(resultList);
    mainLayout->setSpacing(14);
    mainLayout->setContentsMargins(18, 18, 18, 18);
    setLayout(mainLayout);

    QThread *a = new QThread();
    take_pcm = std::make_shared<TakePcm>();
    a->start();
    take_pcm->moveToThread(a);
    take_pcm->setTranslate(true);

    connect(browseButton, &QPushButton::clicked, this, [=]() {
        QString fileName = QFileDialog::getOpenFileName(this, "选择音频文件", "", "音频文件 (*.wav *.mp3 *.aac *.flac)");
        if (!fileName.isEmpty()) {
            config_.audioPath_ = fileName;
            filePathEdit->setText(fileName);
        }
    });

    // 只在独立窗口时连接窗口控制按钮
    if (!parent) {
        connect(maximizeButton, &QPushButton::clicked, this, [=]() {
            this->showMinimized();
        });
        connect(minimizeButton, &QPushButton::clicked, this, [=]() {
            if (isMaximized()) {
                showNormal();
            } else {
                showMaximized();
            }
        });
        connect(closeButton, &QPushButton::clicked, this, [=]() {
            this->close();
        });
    }

    connect(this, &TranslateWidget::signal_begin_tranform, this, &TranslateWidget::on_signal_begin_transform);
    connect(this, &TranslateWidget::signal_erorr, this, &TranslateWidget::showTipMessage);

    connect(this, &TranslateWidget::signal_begin_take_pcm, take_pcm.get(), &TakePcm::signal_begin_make_pcm);
    connect(take_pcm.get(), &TakePcm::signal_send_data, this, &TranslateWidget::on_signal_send_data);
    connect(take_pcm.get(), &TakePcm::signal_decodeEnd, this, &TranslateWidget::on_signal_decodeEnd);
    connect(this, &TranslateWidget::signal_outFile, this, &TranslateWidget::on_signal_outFile);
}
void TranslateWidget::on_signal_decodeEnd(){
    {
        std::lock_guard<std::mutex> lock(mtx);
        translating = true;
    }cv.notify_one();
}
void TranslateWidget::on_signal_send_data(uint8_t *buffer, int bufferSize, qint64 timeMap){
    int16_t* samples = reinterpret_cast<int16_t*>(buffer);
    int n_samples = bufferSize / sizeof(int16_t) / 2; // 每帧采样点数（每通道）
    for (int i = 0; i < n_samples; ++i) {
        // 立体声转单声道（简单平均）
        float left = samples[2*i] / 32768.0f;
        float right = samples[2*i+1] / 32768.0f;
        float mono = (left + right) / 2.0f;
        pcmf32_.append(mono);
    }
    free(buffer);
}
void TranslateWidget::updateProgress(int progress){
    this->progressBar->setValue(progress);
}
void TranslateWidget::on_signal_outFile(const QStringList &segments){

    QFileDialog dialog(this);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setWindowTitle("保存输出");
    dialog.setDirectory("/home/shen");

    // 设置文件后缀：根据config_.outputMode_动态生成
    QString suffix = config_.outputMode_;
    dialog.setDefaultSuffix(suffix);  // 关键设置：自动添加后缀

    // 设置名称过滤器（可选）
    dialog.setNameFilter(QString("%1 files (*.%1)").arg(suffix));

    if (dialog.exec()) {
        QString fileName = dialog.selectedFiles().first();
        QFile outFile(fileName);
        if (outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&outFile);
            for(const QString &segment : segments) {
                out << segment << '\n';
            }
            outFile.close();
            QMessageBox::information(this, "保存成功", "文件已保存");
            
            // 添加到结果列表
            QFileInfo fileInfo(fileName);
            addResultItem(fileInfo.fileName(), fileName);
        }
    }
    transcribeButton->setText("开始转换");
}
void TranslateWidget::showTipMessage(const QString &msg) {
    QMessageBox::information(this, "提示", msg);
}
void TranslateWidget::on_signal_begin_transform(){
    QThread* thread = QThread::create([=](){

        struct whisper_context* ctx = whisper_init_from_file((modelPath + config_.modelName_).toStdString().c_str());
        if (!ctx) {
            emit signal_erorr("模型加载失败");
            return;
        }
        struct whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
        params.print_progress = false;
        params.language = languageMap[config_.language_].toStdString().c_str();  // 或者使用 "chinese"
        params.detect_language = false;  // 禁用自动语言检测
        params.initial_prompt = initialPromptMap[languageMap[config_.language_]];

        qDebug()<<languageMap[config_.language_] << initialPromptMap[languageMap[config_.language_]];

        // 可以尝试调整温度参数，有时能改善输出质量
        params.temperature = 0.1f;
        params.progress_callback = [](struct whisper_context*, struct whisper_state*, int progress, void* user_data) {
            QMetaObject::invokeMethod((QObject*)user_data, "updateProgress", Qt::QueuedConnection, Q_ARG(int, progress));
        };
        params.progress_callback_user_data = this;

        emit signal_begin_take_pcm(config_.audioPath_);

        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [=]{return translating.load();});
        int ret = whisper_full(ctx, params, pcmf32_.data(), pcmf32_.size());
        if (ret != 0) {
            emit signal_erorr("转写失败");
            whisper_free(ctx);
            return;
        }
        QStringList outputLines;
        factory.save(config_.outputMode_,ctx, outputLines);

        emit signal_outFile(outputLines);
        whisper_free(ctx);
        translating = false;
    });
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

// 鼠标拖动窗口实现 - 只在独立窗口时启用
void TranslateWidget::mousePressEvent(QMouseEvent *event)
{
    if (!parent() && event->button() == Qt::LeftButton && event->pos().y() <= 40) {
        mousePressed = true;
        mouseStartPoint = event->globalPos();
        windowStartPoint = this->frameGeometry().topLeft();
    }
    QWidget::mousePressEvent(event);
}

void TranslateWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!parent() && mousePressed) {
        QPoint distance = event->globalPos() - mouseStartPoint;
        this->move(windowStartPoint + distance);
    }
    QWidget::mouseMoveEvent(event);
}
void TranslateWidget::on_transcribeButton_clicked(){
    if(translating.load())
        return;
    transcribeButton->setText("转换中");
    pcmf32_.clear();
    emit signal_begin_tranform();
}
void TranslateWidget::mouseReleaseEvent(QMouseEvent *event)
{
    mousePressed = false;
    QWidget::mouseReleaseEvent(event);
}
void TranslateWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 只在独立窗口时绘制自定义背景
    if (!parent()) {
        // 主体背景：明亮白色，带柔和阴影
        QRect bgRect = rect().adjusted(2, 2, -2, -2);
        QColor bgColor(255, 255, 255, 250);
        painter.setBrush(bgColor);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(bgRect, 16, 16);

        // 顶部渐变条：淡粉到淡绿，柔和且不突兀
        QRect gradRect = QRect(bgRect.left(), bgRect.top(), bgRect.width(), 48);
        QLinearGradient grad(gradRect.topLeft(), gradRect.bottomLeft());
        grad.setColorAt(0, QColor(255, 71, 102, 80));   // 淡粉
        grad.setColorAt(1, QColor(29, 185, 84, 40));    // 淡绿
        painter.setBrush(grad);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(gradRect, 16, 16);

        // 四角圆角遮罩，避免渐变溢出
        QRegion maskRegion(bgRect, QRegion::Ellipse);
        painter.setClipRegion(QRegion(bgRect, QRegion::Rectangle));

        // 保持子控件样式
        QStyleOption opt;
        opt.initFrom(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);
    } else {
        // 内嵌模式使用默认背景
        QWidget::paintEvent(event);
    }
}
void saveTXT(struct whisper_context* ctx, QStringList &outputLines){
    int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char *text = whisper_full_get_segment_text(ctx, i);
        outputLines << QString::fromUtf8(text);
    }
}
void saveJSON(struct whisper_context* ctx, QStringList &outputLines){
    QJsonArray lyricsArray;
    int n_segments = whisper_full_n_segments(ctx);

    for (int i = 0; i < n_segments; ++i) {
        const char *text = whisper_full_get_segment_text(ctx, i);
        int64_t start_time = whisper_full_get_segment_t0(ctx, i);
        int64_t end_time = whisper_full_get_segment_t1(ctx, i);

        QJsonObject lyricObj;
        lyricObj["start"] = static_cast<double>(start_time * 10); // 毫秒
        lyricObj["end"] = static_cast<double>(end_time * 10);     // 毫秒
        lyricObj["text"] = QString::fromUtf8(text).trimmed();

        lyricsArray.append(lyricObj);
    }

    QJsonObject rootObj;
    rootObj["lyrics"] = lyricsArray;
    QJsonDocument doc(rootObj);
    outputLines << doc.toJson();
}
void saveKAR(struct whisper_context* ctx, QStringList &outputLines){
    // KAR 卡拉OK格式（简化版）
    outputLines << "@TITLE Generated Lyrics";
    outputLines << "@V0100";
    outputLines << "@KMIDI KARAOKE FILE";
    outputLines << "";

    int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char *text = whisper_full_get_segment_text(ctx, i);
        int64_t start_time = whisper_full_get_segment_t0(ctx, i);

        // 转换为毫秒
        start_time *= 10;

        // 格式: 分钟:秒:百分秒
        int minutes = start_time / 60000;
        int seconds = (start_time % 60000) / 1000;
        int centiseconds = (start_time % 1000) / 10; // 百分秒

        QString timeTag = QString("%1:%2:%3")
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'))
                .arg(centiseconds, 2, 10, QChar('0'));

        outputLines << timeTag + " [0000][0000][0000] " +
                       QString::fromUtf8(text).trimmed();
    }
}
void saveLRC(struct whisper_context* ctx, QStringList &outputLines){
    int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char *text = whisper_full_get_segment_text(ctx, i);
        int64_t start_time = whisper_full_get_segment_t0(ctx, i);

        int64_t start_ms = start_time * 10;

        int minutes = start_ms / 60000;
        int seconds = (start_ms % 60000) / 1000;
        int centiseconds = (start_ms % 1000) / 10;

        QString timeTag = QString("[%1:%2.%3]")
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'))
                .arg(centiseconds, 2, 10, QChar('0'));

        QString lrcLine = timeTag + QString::fromUtf8(text).trimmed();
        outputLines << lrcLine;
    }
}
void saveVTT(struct whisper_context* ctx, QStringList &outputLines){
    // WebVTT 字幕格式
    outputLines << "WEBVTT";
    outputLines << "";

    int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char *text = whisper_full_get_segment_text(ctx, i);
        int64_t start_time = whisper_full_get_segment_t0(ctx, i);
        int64_t end_time = whisper_full_get_segment_t1(ctx, i);

        // 转换为毫秒
        start_time *= 10;
        end_time *= 10;

        // 格式化时间 (hh:mm:ss.ttt)
        QString start_str = QString("%1:%2:%3.%4")
                .arg(start_time / 3600000, 2, 10, QChar('0'))  // 小时
                .arg((start_time % 3600000) / 60000, 2, 10, QChar('0'))   // 分钟
                .arg((start_time % 60000) / 1000, 2, 10, QChar('0'))      // 秒
                .arg(start_time % 1000, 3, 10, QChar('0'));              // 毫秒

        QString end_str = QString("%1:%2:%3.%4")
                .arg(end_time / 3600000, 2, 10, QChar('0'))
                .arg((end_time % 3600000) / 60000, 2, 10, QChar('0'))
                .arg((end_time % 60000) / 1000, 2, 10, QChar('0'))
                .arg(end_time % 1000, 3, 10, QChar('0'));

        outputLines << QString::number(i + 1);
        outputLines << start_str + " --> " + end_str;
        outputLines << QString::fromUtf8(text).trimmed();
        outputLines << "";
    }
}
void saveSRT(struct whisper_context* ctx, QStringList &outputLines){
    // SRT 字幕格式
    int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char *text = whisper_full_get_segment_text(ctx, i);
        int64_t start_time = whisper_full_get_segment_t0(ctx, i);
        int64_t end_time = whisper_full_get_segment_t1(ctx, i);

        // 转换为毫秒
        start_time *= 10;
        end_time *= 10;

        // 格式化时间 (hh:mm:ss,ttt)
        QString start_str = QString("%1:%2:%3,%4")
                .arg(start_time / 3600000, 2, 10, QChar('0'))  // 小时
                .arg((start_time % 3600000) / 60000, 2, 10, QChar('0'))   // 分钟
                .arg((start_time % 60000) / 1000, 2, 10, QChar('0'))      // 秒
                .arg(start_time % 1000, 3, 10, QChar('0'));              // 毫秒

        QString end_str = QString("%1:%2:%3,%4")
                .arg(end_time / 3600000, 2, 10, QChar('0'))
                .arg((end_time % 3600000) / 60000, 2, 10, QChar('0'))
                .arg((end_time % 60000) / 1000, 2, 10, QChar('0'))
                .arg(end_time % 1000, 3, 10, QChar('0'));

        outputLines << QString::number(i + 1);
        outputLines << start_str + " --> " + end_str;
        outputLines << QString::fromUtf8(text).trimmed();
        outputLines << "";
    }
}

void TranslateWidget::addResultItem(const QString &fileName, const QString &filePath)
{
    // 创建自定义列表项
    ResultListItem* item = new ResultListItem(fileName, filePath, this);
    
    // 创建 QListWidgetItem 并设置自定义控件
    QListWidgetItem* listItem = new QListWidgetItem(resultList);
    listItem->setSizeHint(item->sizeHint());
    
    // 将自定义控件设置到列表项
    resultList->setItemWidget(listItem, item);
    
    // 滚动到最新添加的项
    resultList->scrollToBottom();
}
