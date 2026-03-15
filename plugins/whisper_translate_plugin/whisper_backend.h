#ifndef WHISPER_BACKEND_H
#define WHISPER_BACKEND_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "take_pcm.h"

// 前向声明
struct whisper_context;

/**
 * @brief Whisper翻译后端类 - QML接口
 */
class WhisperBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString audioFile READ audioFile WRITE setAudioFile NOTIFY audioFileChanged)
    Q_PROPERTY(QString modelPath READ modelPath WRITE setModelPath NOTIFY modelPathChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString transcribedText READ transcribedText NOTIFY transcribedTextChanged)
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QStringList supportedLanguages READ supportedLanguages CONSTANT)
    Q_PROPERTY(QStringList availableModels READ availableModels NOTIFY availableModelsChanged)
    Q_PROPERTY(QString outputFormat READ outputFormat WRITE setOutputFormat NOTIFY outputFormatChanged)

public:
    explicit WhisperBackend(QObject *parent = nullptr);
    ~WhisperBackend();

    // Property getters
    QString audioFile() const { return m_audioFile; }
    QString modelPath() const { return m_modelPath; }
    QString language() const { return m_language; }
    QString transcribedText() const { return m_transcribedText; }
    bool isProcessing() const { return m_isProcessing; }
    int progress() const { return m_progress; }
    QString statusText() const { return m_statusText; }
    QStringList supportedLanguages() const { return m_supportedLanguages; }
    QStringList availableModels() const { return m_availableModels; }
    QString outputFormat() const { return m_outputFormat; }

    void setAudioFile(const QString &file);
    void setModelPath(const QString &path);
    void setLanguage(const QString &lang);
    void setOutputFormat(const QString &format);

public slots:
    // QML可调用的方法
    void selectAudioFile();
    void selectModelFile();
    void startTranscription();
    void stopTranscription();
    void clearText();
    void saveTranscription();
    void scanForModels();
    
    // 进度更新槽
    void updateProgressSlot(int value);

signals:
    void audioFileChanged();
    void modelPathChanged();
    void languageChanged();
    void transcribedTextChanged();
    void isProcessingChanged();
    void progressChanged();
    void statusTextChanged();
    void availableModelsChanged();
    void transcriptionFinished(bool success, const QString &message);
    void outputFormatChanged();
    
    // 内部信号
    void signalBeginTakePcm(const QString &audioPath);
    void signalOutFile(const QStringList &segments);

private slots:
    void onSendData(uint8_t *buffer, int bufferSize, qint64 timeMap);
    void onDecodeEnd();
    void updateStatus(const QString &text);
    void updateProgress(int value);
    void setTranscribedText(const QString &text);
    void setIsProcessing(bool processing);
    void performTranscription();

private:
    // 输出行结构体
    struct OutputLine {
        int64_t t0;
        int64_t t1;
        QString text;
    };
    
    // 格式转换函数
    void saveTXT(const QString& filename, const QVector<OutputLine>& lines);
    void saveJSON(const QString& filename, const QVector<OutputLine>& lines);
    void saveLRC(const QString& filename, const QVector<OutputLine>& lines);
    void saveVTT(const QString& filename, const QVector<OutputLine>& lines);
    void saveSRT(const QString& filename, const QVector<OutputLine>& lines);

private:
    QString m_audioFile;
    QString m_modelPath;
    QString m_language;
    QString m_outputFormat;
    QString m_transcribedText;
    bool m_isProcessing;
    int m_progress;
    QString m_statusText;
    QStringList m_supportedLanguages;
    QStringList m_availableModels;
    
    // Whisper 相关
    QVector<float> pcmf32_;
    QVector<OutputLine> lastOutputLines_;  // 存储最后的转录结果（带时间戳）
    std::shared_ptr<TakePcm> take_pcm;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> translating{false};
    std::atomic<bool> shouldStop{false};
};

#endif // WHISPER_BACKEND_H
