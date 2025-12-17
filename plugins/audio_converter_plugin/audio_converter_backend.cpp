#include "audio_converter_backend.h"
#include <QFileDialog>
#include <QDebug>
#include <QStandardPaths>
#include <QThread>

AudioConverterBackend::AudioConverterBackend(QObject *parent)
    : QObject(parent)
    , m_bitrate(192)
    , m_sampleRate(44100)
    , m_isConverting(false)
    , m_progress(0)
{
    m_supportedFormats << "MP3" << "WAV" << "FLAC" << "AAC" << "OGG" << "M4A" << "WMA";
    m_outputFormat = "MP3";
    m_statusText = "就绪";
}

AudioConverterBackend::~AudioConverterBackend()
{
}

void AudioConverterBackend::setInputFile(const QString &file)
{
    if (m_inputFile != file) {
        m_inputFile = file;
        emit inputFileChanged();
        
        // 自动设置输出文件名
        if (!file.isEmpty() && m_outputFile.isEmpty()) {
            QFileInfo fileInfo(file);
            QString baseName = fileInfo.completeBaseName();
            QString outputPath = fileInfo.absolutePath() + "/" + baseName + "_converted." + m_outputFormat.toLower();
            setOutputFile(outputPath);
        }
    }
}

void AudioConverterBackend::setOutputFile(const QString &file)
{
    if (m_outputFile != file) {
        m_outputFile = file;
        emit outputFileChanged();
    }
}

void AudioConverterBackend::setOutputFormat(const QString &format)
{
    if (m_outputFormat != format) {
        m_outputFormat = format;
        emit outputFormatChanged();
        
        // 更新输出文件扩展名
        if (!m_outputFile.isEmpty()) {
            QFileInfo fileInfo(m_outputFile);
            QString newOutput = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + "." + format.toLower();
            setOutputFile(newOutput);
        }
    }
}

void AudioConverterBackend::setBitrate(int bitrate)
{
    if (m_bitrate != bitrate) {
        m_bitrate = bitrate;
        emit bitrateChanged();
    }
}

void AudioConverterBackend::setSampleRate(int rate)
{
    if (m_sampleRate != rate) {
        m_sampleRate = rate;
        emit sampleRateChanged();
    }
}

void AudioConverterBackend::selectInputFile()
{
    QString musicPath = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    QString fileName = QFileDialog::getOpenFileName(
        nullptr,
        "选择音频文件",
        musicPath,
        "音频文件 (*.mp3 *.wav *.flac *.aac *.ogg *.m4a *.wma);;所有文件 (*.*)"
    );
    
    if (!fileName.isEmpty()) {
        setInputFile(fileName);
        updateStatus("已选择输入文件: " + QFileInfo(fileName).fileName());
    }
}

void AudioConverterBackend::selectOutputFile()
{
    QString musicPath = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    QString filter = QString("%1 文件 (*.%2);;所有文件 (*.*)").arg(m_outputFormat).arg(m_outputFormat.toLower());
    
    QString fileName = QFileDialog::getSaveFileName(
        nullptr,
        "保存转换后的文件",
        m_outputFile.isEmpty() ? musicPath : m_outputFile,
        filter
    );
    
    if (!fileName.isEmpty()) {
        setOutputFile(fileName);
        updateStatus("已选择输出文件: " + QFileInfo(fileName).fileName());
    }
}

void AudioConverterBackend::startConversion()
{
    if (m_inputFile.isEmpty()) {
        updateStatus("错误: 请先选择输入文件");
        emit conversionFinished(false, "未选择输入文件");
        return;
    }
    
    if (m_outputFile.isEmpty()) {
        updateStatus("错误: 请先选择输出文件");
        emit conversionFinished(false, "未选择输出文件");
        return;
    }
    
    m_isConverting = true;
    emit isConvertingChanged();
    
    updateStatus("正在转换...");
    updateProgress(0);
    
    // TODO: 实际的音频转换逻辑
    // 这里使用 FFmpeg 进行转换
    // 示例代码:
    qDebug() << "Converting:" << m_inputFile << "to" << m_outputFile;
    qDebug() << "Format:" << m_outputFormat << "Bitrate:" << m_bitrate << "SampleRate:" << m_sampleRate;
    
    // 模拟转换进度
    for (int i = 0; i <= 100; i += 10) {
        updateProgress(i);
        QThread::msleep(100);
    }
    
    m_isConverting = false;
    emit isConvertingChanged();
    
    updateStatus("转换完成!");
    emit conversionFinished(true, "转换成功");
}

void AudioConverterBackend::stopConversion()
{
    if (m_isConverting) {
        m_isConverting = false;
        emit isConvertingChanged();
        updateStatus("转换已取消");
        emit conversionFinished(false, "用户取消");
    }
}

void AudioConverterBackend::clearAll()
{
    setInputFile("");
    setOutputFile("");
    updateProgress(0);
    updateStatus("就绪");
}

void AudioConverterBackend::updateStatus(const QString &text)
{
    m_statusText = text;
    emit statusTextChanged();
}

void AudioConverterBackend::updateProgress(int value)
{
    m_progress = value;
    emit progressChanged();
}
