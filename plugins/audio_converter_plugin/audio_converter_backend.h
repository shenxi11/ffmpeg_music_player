#ifndef AUDIO_CONVERTER_BACKEND_H
#define AUDIO_CONVERTER_BACKEND_H

#include <QObject>
#include <QString>
#include <QStringList>

/**
 * @brief 音频转换器后端类 - QML接口
 */
class AudioConverterBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString inputFile READ inputFile WRITE setInputFile NOTIFY inputFileChanged)
    Q_PROPERTY(QString outputFile READ outputFile WRITE setOutputFile NOTIFY outputFileChanged)
    Q_PROPERTY(QString outputFormat READ outputFormat WRITE setOutputFormat NOTIFY outputFormatChanged)
    Q_PROPERTY(int bitrate READ bitrate WRITE setBitrate NOTIFY bitrateChanged)
    Q_PROPERTY(int sampleRate READ sampleRate WRITE setSampleRate NOTIFY sampleRateChanged)
    Q_PROPERTY(bool isConverting READ isConverting NOTIFY isConvertingChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QStringList supportedFormats READ supportedFormats CONSTANT)

public:
    explicit AudioConverterBackend(QObject *parent = nullptr);
    ~AudioConverterBackend();

    // Property getters
    QString inputFile() const { return m_inputFile; }
    QString outputFile() const { return m_outputFile; }
    QString outputFormat() const { return m_outputFormat; }
    int bitrate() const { return m_bitrate; }
    int sampleRate() const { return m_sampleRate; }
    bool isConverting() const { return m_isConverting; }
    int progress() const { return m_progress; }
    QString statusText() const { return m_statusText; }
    QStringList supportedFormats() const { return m_supportedFormats; }

    // Property setters
    void setInputFile(const QString &file);
    void setOutputFile(const QString &file);
    void setOutputFormat(const QString &format);
    void setBitrate(int bitrate);
    void setSampleRate(int rate);

public slots:
    // QML可调用的方法
    void selectInputFile();
    void selectOutputFile();
    void startConversion();
    void stopConversion();
    void clearAll();

signals:
    void inputFileChanged();
    void outputFileChanged();
    void outputFormatChanged();
    void bitrateChanged();
    void sampleRateChanged();
    void isConvertingChanged();
    void progressChanged();
    void statusTextChanged();
    void conversionFinished(bool success, const QString &message);

private:
    void updateStatus(const QString &text);
    void updateProgress(int value);

    QString m_inputFile;
    QString m_outputFile;
    QString m_outputFormat;
    int m_bitrate;
    int m_sampleRate;
    bool m_isConverting;
    int m_progress;
    QString m_statusText;
    QStringList m_supportedFormats;
};

#endif // AUDIO_CONVERTER_BACKEND_H
