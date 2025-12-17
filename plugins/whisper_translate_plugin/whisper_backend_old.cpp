#include "whisper_backend.h"
#include <QFileDialog>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QThread>

WhisperBackend::WhisperBackend(QObject *parent)
    : QObject(parent)
    , m_isProcessing(false)
    , m_progress(0)
    , m_language("auto")
{
    m_supportedLanguages << "自动检测(auto)" << "中文(zh)" << "英文(en)" 
                         << "日文(ja)" << "韩文(ko)" << "法文(fr)" 
                         << "德文(de)" << "西班牙文(es)" << "俄文(ru)";
    
    m_statusText = "就绪";
    
    // 扫描可用的模型
    scanForModels();
}

WhisperBackend::~WhisperBackend()
{
}

void WhisperBackend::setAudioFile(const QString &file)
{
    if (m_audioFile != file) {
        m_audioFile = file;
        emit audioFileChanged();
        if (!file.isEmpty()) {
            updateStatus("已选择音频文件: " + QFileInfo(file).fileName());
        }
    }
}

void WhisperBackend::setModelPath(const QString &path)
{
    if (m_modelPath != path) {
        m_modelPath = path;
        emit modelPathChanged();
        if (!path.isEmpty()) {
            updateStatus("已选择模型: " + QFileInfo(path).fileName());
        }
    }
}

void WhisperBackend::setLanguage(const QString &lang)
{
    if (m_language != lang) {
        m_language = lang;
        emit languageChanged();
    }
}

void WhisperBackend::setTranscribedText(const QString &text)
{
    if (m_transcribedText != text) {
        m_transcribedText = text;
        emit transcribedTextChanged();
    }
}

void WhisperBackend::selectAudioFile()
{
    QString musicPath = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    QString fileName = QFileDialog::getOpenFileName(
        nullptr,
        "选择音频文件",
        musicPath,
        "音频文件 (*.mp3 *.wav *.flac *.m4a *.ogg);;所有文件 (*.*)"
    );
    
    if (!fileName.isEmpty()) {
        setAudioFile(fileName);
    }
}

void WhisperBackend::selectModelFile()
{
    QString fileName = QFileDialog::getOpenFileName(
        nullptr,
        "选择Whisper模型文件",
        QDir::currentPath(),
        "模型文件 (*.bin);;所有文件 (*.*)"
    );
    
    if (!fileName.isEmpty()) {
        setModelPath(fileName);
    }
}

void WhisperBackend::startTranscription()
{
    if (m_audioFile.isEmpty()) {
        updateStatus("错误: 请先选择音频文件");
        emit transcriptionFinished(false, "未选择音频文件");
        return;
    }
    
    if (m_modelPath.isEmpty()) {
        updateStatus("错误: 请先选择模型文件");
        emit transcriptionFinished(false, "未选择模型文件");
        return;
    }
    
    m_isProcessing = true;
    emit isProcessingChanged();
    
    updateStatus("正在转录...");
    updateProgress(0);
    
    // TODO: 实际的Whisper转录逻辑
    qDebug() << "Transcribing:" << m_audioFile;
    qDebug() << "Model:" << m_modelPath;
    qDebug() << "Language:" << m_language;
    
    // 模拟转录进度
    QString result = "这是模拟的转录结果。\n\n";
    result += "实际使用时，这里会调用 Whisper.cpp 进行音频转文字。\n";
    result += "支持多种语言的语音识别。\n\n";
    result += "音频文件: " + QFileInfo(m_audioFile).fileName() + "\n";
    result += "使用模型: " + QFileInfo(m_modelPath).fileName() + "\n";
    result += "目标语言: " + m_language;
    
    for (int i = 0; i <= 100; i += 10) {
        updateProgress(i);
        QThread::msleep(200);
    }
    
    setTranscribedText(result);
    
    m_isProcessing = false;
    emit isProcessingChanged();
    
    updateStatus("转录完成!");
    emit transcriptionFinished(true, "转录成功");
}

void WhisperBackend::stopTranscription()
{
    if (m_isProcessing) {
        m_isProcessing = false;
        emit isProcessingChanged();
        updateStatus("转录已取消");
        emit transcriptionFinished(false, "用户取消");
    }
}

void WhisperBackend::clearText()
{
    setTranscribedText("");
    updateProgress(0);
    updateStatus("就绪");
}

void WhisperBackend::saveTranscription()
{
    if (m_transcribedText.isEmpty()) {
        updateStatus("没有可保存的内容");
        return;
    }
    
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString fileName = QFileDialog::getSaveFileName(
        nullptr,
        "保存转录文本",
        documentsPath + "/transcription.txt",
        "文本文件 (*.txt);;所有文件 (*.*)"
    );
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << m_transcribedText;
            file.close();
            updateStatus("已保存: " + QFileInfo(fileName).fileName());
        } else {
            updateStatus("错误: 无法保存文件");
        }
    }
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

void WhisperBackend::updateStatus(const QString &text)
{
    m_statusText = text;
    emit statusTextChanged();
}

void WhisperBackend::updateProgress(int value)
{
    m_progress = value;
    emit progressChanged();
}
