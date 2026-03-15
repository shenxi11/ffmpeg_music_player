#include "whisper_backend.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <QThread>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "whisper.h"

// 语言映射
static QMap<QString, QString> languageCodeMap = {
    {"中文 (zh)", "zh"},
    {"英文 (en)", "en"},
    {"日语 (ja)", "ja"},
    {"韩语 (ko)", "ko"},
    {"法语 (fr)", "fr"},
    {"德语 (de)", "de"},
    {"西班牙语 (es)", "es"}
};

// 初始提示词映射
static QMap<QString, const char*> initialPromptMap = {
    {"zh", "以下是普通话的句子"},
    {"en", "The following is English text"},
    {"ja", "以下は日本語の文章です"},
    {"ko", "다음은 한국어 문장입니다"},
    {"fr", "Voici une phrase en français"},
    {"de", "Folgendes ist ein deutscher Satz"},
    {"es", "Lo siguiente es una oración en español"}
};

WhisperBackend::WhisperBackend(QObject *parent)
    : QObject(parent)
    , m_isProcessing(false)
    , m_progress(0)
    , m_language("中文 (zh)")
    , m_outputFormat("lrc")
{
    // 初始化支持的语言列表
    m_supportedLanguages = languageCodeMap.keys();
    
    // 创建 TakePcm 对象用于音频解码
    QThread *pcmThread = new QThread();
    take_pcm = std::make_shared<TakePcm>();
    pcmThread->start();
    take_pcm->moveToThread(pcmThread);
    take_pcm->setTranslate(true);
    
    // 连接信号
    connect(this, &WhisperBackend::signalBeginTakePcm, take_pcm.get(), &TakePcm::signalBeginMakePcm);
    connect(take_pcm.get(), &TakePcm::signalSendData, this, &WhisperBackend::onSendData);
    connect(take_pcm.get(), &TakePcm::signalDecodeEnd, this, &WhisperBackend::onDecodeEnd);
    
    // 扫描可用模型
    scanForModels();
    
    updateStatus("就绪");
}

WhisperBackend::~WhisperBackend()
{
}

void WhisperBackend::setAudioFile(const QString &file)
{
    if (m_audioFile != file) {
        m_audioFile = file;
        emit audioFileChanged();
    }
}

void WhisperBackend::setModelPath(const QString &path)
{
    if (m_modelPath != path) {
        m_modelPath = path;
        emit modelPathChanged();
    }
}

void WhisperBackend::setLanguage(const QString &lang)
{
    if (m_language != lang) {
        m_language = lang;
        emit languageChanged();
    }
}

void WhisperBackend::setOutputFormat(const QString &format)
{
    if (m_outputFormat != format) {
        m_outputFormat = format;
        emit outputFormatChanged();
    }
}

void WhisperBackend::setIsProcessing(bool processing)
{
    if (m_isProcessing != processing) {
        m_isProcessing = processing;
        emit isProcessingChanged();
    }
}

void WhisperBackend::selectAudioFile()
{
    QString fileName = QFileDialog::getOpenFileName(
        nullptr,
        "选择音频文件",
        "",
        "音频文件 (*.wav *.mp3 *.aac *.flac *.m4a *.ogg)"
    );
    
    if (!fileName.isEmpty()) {
        setAudioFile(fileName);
        updateStatus("已选择: " + QFileInfo(fileName).fileName());
    }
}

void WhisperBackend::selectModelFile()
{
    QString fileName = QFileDialog::getOpenFileName(
        nullptr,
        "选择 Whisper 模型文件",
        "E:/FFmpeg_whisper/ggml-models",
        "模型文件 (*.bin)"
    );
    
    if (!fileName.isEmpty()) {
        setModelPath(fileName);
        updateStatus("已选择模型: " + QFileInfo(fileName).fileName());
    }
}

void WhisperBackend::startTranscription()
{
    if (m_isProcessing) {
        return;
    }
    
    if (m_audioFile.isEmpty()) {
        updateStatus("错误: 请先选择音频文件");
        emit transcriptionFinished(false, "请先选择音频文件");
        return;
    }
    
    if (m_modelPath.isEmpty()) {
        updateStatus("错误: 请先选择模型文件");
        emit transcriptionFinished(false, "请先选择模型文件");
        return;
    }
    
    m_isProcessing = true;
    emit isProcessingChanged();
    shouldStop = false;
    
    updateStatus("准备转录...");
    updateProgress(0);
    
    // 直接调用performTranscription,它内部会创建线程
    performTranscription();
}

void WhisperBackend::performTranscription()
{
    qDebug() << "[performTranscription] Starting transcription thread...";
    // 创建独立线程执行转录
    QThread* thread = QThread::create([this]() {
        qDebug() << "[Thread] Transcription thread started";
        // 加载模型
        updateStatus("正在加载模型...");
        qDebug() << "[Thread] Loading model from:" << m_modelPath;
        struct whisper_context* ctx = whisper_init_from_file(m_modelPath.toUtf8().constData());
        
        if (!ctx) {
            qDebug() << "[Thread] ERROR: Model loading failed!";
            updateStatus("错误: 模型加载失败");
            QMetaObject::invokeMethod(this, [this]() {
                setIsProcessing(false);
                emit transcriptionFinished(false, "模型加载失败");
            }, Qt::QueuedConnection);
            return;
        }
        qDebug() << "[Thread] Model loaded successfully";
        
        // 设置参数
        struct whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
        params.print_progress = false;
        
        // 提取语言代码 - 使用局部变量保存避免临时对象销毁
        QString langCode = languageCodeMap.value(m_language, "zh");
        std::string langStr = langCode.toStdString();  // 保存为std::string
        const char* promptStr = initialPromptMap.value(langCode, "");
        
        params.language = langStr.c_str();
        params.detect_language = false;
        params.initial_prompt = promptStr;
        params.temperature = 0.4f;  // 增加温度避免重复输出
        params.temperature_inc = 0.2f;  // 失败时增加温度
        params.translate = false;
        params.no_context = false;
        params.single_segment = false;
        params.token_timestamps = true;
        params.split_on_word = true;
        params.suppress_blank = true;
        params.entropy_thold = 2.4f;  // 熵阈值
        params.logprob_thold = -1.0f;  // 对数概率阈值
        params.no_speech_thold = 0.6f;  // 无语音阈值
        params.print_realtime = false;
        params.print_progress = false;
        
        qDebug() << "[Thread] Language:" << langCode << "Prompt:" << promptStr;
        qDebug() << "[Thread] Whisper params: temp=" << params.temperature 
                 << "temp_inc=" << params.temperature_inc
                 << "entropy_thold=" << params.entropy_thold
                 << "no_speech_thold=" << params.no_speech_thold;
        
        // 进度回调
        params.progress_callback = [](struct whisper_context*, struct whisper_state*, int progress, void* user_data) {
            WhisperBackend* backend = static_cast<WhisperBackend*>(user_data);
            QMetaObject::invokeMethod(backend, "updateProgressSlot", Qt::QueuedConnection, Q_ARG(int, progress));
        };
        params.progress_callback_user_data = this;
        
        // 清空 PCM 数据并开始解码音频
        pcmf32_.clear();
        translating = false;
        
        updateStatus("正在解码音频...");
        qDebug() << "[Thread] Starting audio decode, emitting signalBeginTakePcm...";
        
        // 启动音频解码(异步)
        emit signalBeginTakePcm(m_audioFile);
        
        // 等待音频解码完成
        qDebug() << "[Thread] Waiting for audio decode to complete...";
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]{ return translating.load() || shouldStop.load(); });
        qDebug() << "[Thread] Wait finished. translating=" << translating.load() << " shouldStop=" << shouldStop.load();
        
        if (shouldStop) {
            qDebug() << "[Thread] Transcription cancelled by user";
            whisper_free(ctx);
            updateStatus("已取消");
            QMetaObject::invokeMethod(this, [this]() {
                setIsProcessing(false);
                emit transcriptionFinished(false, "用户取消");
            }, Qt::QueuedConnection);
            return;
        }
        
        qDebug() << "[Thread] Audio decoding finished, total samples:" << pcmf32_.size();
        updateStatus("音频解码完成,开始转录...");
        QMetaObject::invokeMethod(this, [this]() { updateProgress(10); }, Qt::QueuedConnection);
        
        // 执行转录
        qDebug() << "[Thread] Calling whisper_full with" << pcmf32_.size() << "samples...";
        int ret = whisper_full(ctx, params, pcmf32_.data(), pcmf32_.size());
        qDebug() << "[Thread] whisper_full returned:" << ret;
        
        if (ret != 0) {
            updateStatus("错误: 转录失败");
            whisper_free(ctx);
            QMetaObject::invokeMethod(this, [this]() {
                setIsProcessing(false);
                emit transcriptionFinished(false, "转录失败");
            }, Qt::QueuedConnection);
            return;
        }
        
        // 提取转录结果
        int n_segments = whisper_full_n_segments(ctx);
        qDebug() << "Transcription completed, segments:" << n_segments;
        
        // 检查是否有实际内容
        if (n_segments == 0) {
            qDebug() << "[Thread] WARNING: No segments detected!";
            whisper_free(ctx);
            QMetaObject::invokeMethod(this, [this]() {
                setIsProcessing(false);
                updateStatus("警告: 未检测到语音内容");
                emit transcriptionFinished(false, "未检测到语音内容");
            }, Qt::QueuedConnection);
            return;
        }
        
        // 保存转录结果到OutputLine结构
        QVector<OutputLine> outputLines;
        QString resultText;
        qDebug() << "\n========== 转录结果 ==========";
        for (int i = 0; i < n_segments; ++i) {
            const char *text = whisper_full_get_segment_text(ctx, i);
            int64_t t0 = whisper_full_get_segment_t0(ctx, i);
            int64_t t1 = whisper_full_get_segment_t1(ctx, i);
            
            // 打印详细的段信息
            QString textStr = QString::fromUtf8(text).trimmed();
            qDebug() << QString("[Segment %1] t0=%2ms, t1=%3ms, duration=%4ms, text=\"%5\"")
                        .arg(i)
                        .arg(t0 * 10)
                        .arg(t1 * 10)
                        .arg((t1 - t0) * 10)
                        .arg(textStr);
            
            OutputLine line;
            line.t0 = t0;
            line.t1 = t1;
            line.text = textStr;
            outputLines.append(line);
            
            // 构建预览文本(前10段)
            if (i < 10) {
                resultText += line.text;
                if (i < n_segments - 1 && i < 9) {
                    resultText += "\n";
                }
            }
        }
        qDebug() << "============================\n";
        
        // 显示音频和转录统计信息
        float audio_duration_sec = pcmf32_.size() / 16000.0f;
        qDebug() << "[Thread] Audio duration:" << audio_duration_sec << "seconds";
        qDebug() << "[Thread] Total segments:" << n_segments;
        qDebug() << "[Thread] Average segment duration:" << (audio_duration_sec / n_segments) << "seconds";
        
        if (n_segments > 10) {
            resultText += "\n\n... (共" + QString::number(n_segments) + "段)";
        }
        
        whisper_free(ctx);
        
        // 在主线程更新UI
        QMetaObject::invokeMethod(this, [this, outputLines, resultText, n_segments]() {
            qDebug() << "Updating UI with transcription results...";
            lastOutputLines_ = outputLines;
            setTranscribedText(resultText);
            updateStatus("转录完成! 点击'保存'按钮保存文件");
            updateProgress(100);
            setIsProcessing(false);
            translating = false;
            qDebug() << "UI updated successfully. Transcription finished with" << n_segments << "segments";
            emit transcriptionFinished(true, "转录成功,共 " + QString::number(n_segments) + " 段");
        }, Qt::QueuedConnection);
    });
    
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void WhisperBackend::stopTranscription()
{
    if (m_isProcessing) {
        shouldStop = true;
        cv.notify_one();
        updateStatus("正在取消...");
    }
}

void WhisperBackend::clearText()
{
    setTranscribedText("");
    updateStatus("已清空");
}

void WhisperBackend::saveTranscription()
{
    if (lastOutputLines_.isEmpty()) {
        updateStatus("没有可保存的内容,请先转录");
        return;
    }
    
    // 获取默认保存路径(音频文件同目录)
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    if (!m_audioFile.isEmpty()) {
        QFileInfo audioInfo(m_audioFile);
        defaultPath = audioInfo.absolutePath() + "/" + audioInfo.baseName() + "." + m_outputFormat;
    }
    
    QString fileName = QFileDialog::getSaveFileName(
        nullptr,
        "保存转录结果",
        defaultPath,
        QString("%1 文件 (*.%2);;所有文件 (*.*)").arg(m_outputFormat.toUpper()).arg(m_outputFormat)
    );
    
    if (fileName.isEmpty()) {
        return;
    }
    
    // 确保文件扩展名正确
    QFileInfo fileInfo(fileName);
    if (fileInfo.suffix().toLower() != m_outputFormat.toLower()) {
        fileName += "." + m_outputFormat;
    }
    
    // 根据格式调用相应的保存函数
    if (m_outputFormat == "txt") {
        saveTXT(fileName, lastOutputLines_);
    } else if (m_outputFormat == "json") {
        saveJSON(fileName, lastOutputLines_);
    } else if (m_outputFormat == "lrc") {
        saveLRC(fileName, lastOutputLines_);
    } else if (m_outputFormat == "vtt") {
        saveVTT(fileName, lastOutputLines_);
    } else if (m_outputFormat == "srt") {
        saveSRT(fileName, lastOutputLines_);
    } else {
        saveLRC(fileName, lastOutputLines_); // 默认LRC格式
    }
    
    updateStatus("已保存: " + fileInfo.fileName());
    QMessageBox::information(nullptr, "保存成功", "文件已保存到:\n" + fileName);
}

void WhisperBackend::scanForModels()
{
    m_availableModels.clear();
    
    // 扫描常见的模型路径
    QStringList searchPaths;
    searchPaths << "E:/FFmpeg_whisper/ggml-models"  // 主要模型路径
                << QDir::currentPath() + "/models"
                << QDir::currentPath() + "/whisper_models"
                << QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/whisper_models";
    
    for (const QString &path : searchPaths) {
        QDir dir(path);
        if (dir.exists()) {
            QStringList filters;
            filters << "*.bin";
            QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
            for (const QFileInfo &fileInfo : files) {
                m_availableModels << fileInfo.absoluteFilePath();
            }
        }
    }
    
    if (!m_availableModels.isEmpty()) {
        setModelPath(m_availableModels.first());
    }
    
    emit availableModelsChanged();
}

void WhisperBackend::updateProgressSlot(int value)
{
    updateProgress(value);
}

void WhisperBackend::onSendData(uint8_t *buffer, int bufferSize, qint64 timeMap)
{
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
    
    // 每收到一定量的数据就更新进度(假设总时长未知,只显示已解码)
    static int lastLoggedSize = 0;
    if (pcmf32_.size() - lastLoggedSize > 160000) { // 约每10秒打印一次(16kHz采样率)
        qDebug() << "Decoded audio data, total samples:" << pcmf32_.size() 
                 << "duration:" << (pcmf32_.size() / 16000.0) << "seconds";
        lastLoggedSize = pcmf32_.size();
        // 更新状态显示已解码的时长
        int seconds = pcmf32_.size() / 16000;
        updateStatus(QString("正在解码音频... 已解码 %1 秒").arg(seconds));
    }
}

void WhisperBackend::onDecodeEnd()
{
    // 防止重复触发
    if (translating.load()) {
        //qDebug() << "Already translating, ignoring decode end signal";
        return;
    }
    
    qDebug() << "Audio decoding completed, total samples:" << pcmf32_.size();
    {
        std::lock_guard<std::mutex> lock(mtx);
        translating = true;
    }
    cv.notify_one();
}

void WhisperBackend::updateStatus(const QString &text)
{
    if (m_statusText != text) {
        m_statusText = text;
        emit statusTextChanged();
    }
}

void WhisperBackend::updateProgress(int value)
{
    if (m_progress != value) {
        m_progress = value;
        emit progressChanged();
    }
}

void WhisperBackend::setTranscribedText(const QString &text)
{
    if (m_transcribedText != text) {
        m_transcribedText = text;
        emit transcribedTextChanged();
    }
}

// ============ 格式转换函数 ============

void WhisperBackend::saveTXT(const QString& filename, const QVector<OutputLine>& lines)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << filename;
        return;
    }
    
    QTextStream out(&file);
    out.setCodec("UTF-8");
    for (const auto& line : lines) {
        out << line.text << "\n";
    }
}

void WhisperBackend::saveJSON(const QString& filename, const QVector<OutputLine>& lines)
{
    QJsonArray lyricsArray;
    
    for (const auto& line : lines) {
        QJsonObject lyricObj;
        lyricObj["start"] = static_cast<double>(line.t0 * 10); // 毫秒
        lyricObj["end"] = static_cast<double>(line.t1 * 10);   // 毫秒
        lyricObj["text"] = line.text.trimmed();
        lyricsArray.append(lyricObj);
    }
    
    QJsonObject rootObj;
    rootObj["lyrics"] = lyricsArray;
    QJsonDocument doc(rootObj);
    
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << filename;
        return;
    }
    
    file.write(doc.toJson());
}

void WhisperBackend::saveLRC(const QString& filename, const QVector<OutputLine>& lines)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << filename;
        return;
    }
    
    QTextStream out(&file);
    out.setCodec("UTF-8");
    
    for (const auto& line : lines) {
        int64_t start_ms = line.t0 * 10;
        int minutes = start_ms / 60000;
        int seconds = (start_ms % 60000) / 1000;
        int centiseconds = (start_ms % 1000) / 10;

        QString timeTag = QString("[%1:%2.%3]")
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'))
                .arg(centiseconds, 2, 10, QChar('0'));

        out << timeTag << line.text.trimmed() << "\n";
    }
}

void WhisperBackend::saveVTT(const QString& filename, const QVector<OutputLine>& lines)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << filename;
        return;
    }
    
    QTextStream out(&file);
    out.setCodec("UTF-8");
    
    // WebVTT 字幕格式
    out << "WEBVTT\n\n";

    for (int i = 0; i < lines.size(); ++i) {
        const auto& line = lines[i];
        
        // 转换为毫秒
        int64_t start_time = line.t0 * 10;
        int64_t end_time = line.t1 * 10;

        // 格式化时间 (hh:mm:ss.ttt)
        QString start_str = QString("%1:%2:%3.%4")
                .arg(start_time / 3600000, 2, 10, QChar('0'))
                .arg((start_time % 3600000) / 60000, 2, 10, QChar('0'))
                .arg((start_time % 60000) / 1000, 2, 10, QChar('0'))
                .arg(start_time % 1000, 3, 10, QChar('0'));

        QString end_str = QString("%1:%2:%3.%4")
                .arg(end_time / 3600000, 2, 10, QChar('0'))
                .arg((end_time % 3600000) / 60000, 2, 10, QChar('0'))
                .arg((end_time % 60000) / 1000, 2, 10, QChar('0'))
                .arg(end_time % 1000, 3, 10, QChar('0'));

        out << QString::number(i + 1) << "\n";
        out << start_str << " --> " << end_str << "\n";
        out << line.text.trimmed() << "\n\n";
    }
}

void WhisperBackend::saveSRT(const QString& filename, const QVector<OutputLine>& lines)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << filename;
        return;
    }
    
    QTextStream out(&file);
    out.setCodec("UTF-8");
    
    // SRT 字幕格式
    for (int i = 0; i < lines.size(); ++i) {
        const auto& line = lines[i];
        
        // 转换为毫秒
        int64_t start_time = line.t0 * 10;
        int64_t end_time = line.t1 * 10;

        // 格式化时间 (hh:mm:ss,ttt)
        QString start_str = QString("%1:%2:%3,%4")
                .arg(start_time / 3600000, 2, 10, QChar('0'))
                .arg((start_time % 3600000) / 60000, 2, 10, QChar('0'))
                .arg((start_time % 60000) / 1000, 2, 10, QChar('0'))
                .arg(start_time % 1000, 3, 10, QChar('0'));

        QString end_str = QString("%1:%2:%3,%4")
                .arg(end_time / 3600000, 2, 10, QChar('0'))
                .arg((end_time % 3600000) / 60000, 2, 10, QChar('0'))
                .arg((end_time % 60000) / 1000, 2, 10, QChar('0'))
                .arg(end_time % 1000, 3, 10, QChar('0'));

        out << QString::number(i + 1) << "\n";
        out << start_str << " --> " << end_str << "\n";
        out << line.text.trimmed() << "\n\n";
    }
}
