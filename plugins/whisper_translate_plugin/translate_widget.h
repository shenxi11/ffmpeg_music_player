#ifndef TRANSLATEWIDGET_H
#define TRANSLATEWIDGET_H

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMap>
#include <QProgressBar>
#include <QPushButton>
#include <QStringList>
#include <QTextEdit>
#include <QThread>
#include <QVector>
#include <QVBoxLayout>
#include <QWidget>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>

#include "take_pcm.h"
#include "whisper.h"

void saveTXT(struct whisper_context* ctx, QStringList &outputLines);
void saveJSON(struct whisper_context* ctx, QStringList &outputLines);
void saveKAR(struct whisper_context* ctx, QStringList &outputLines);
void saveLRC(struct whisper_context* ctx, QStringList &outputLines);
void saveVTT(struct whisper_context* ctx, QStringList &outputLines);
void saveSRT(struct whisper_context* ctx, QStringList &outputLines);

class transformFactory {
public:
    transformFactory();
    void save(const QString &name, struct whisper_context* ctx, QStringList &outputLines);

private:
    std::map<QString, std::function<void(struct whisper_context* ctx, QStringList &outputLines)>> funcMp;
};

class ResultListItem : public QWidget
{
    Q_OBJECT
public:
    explicit ResultListItem(const QString &fileName, const QString &filePath, QWidget *parent = nullptr);

private slots:
    void onOpenFolderClicked();
    void onOpenFileClicked();

private:
    QString fileName;
    QString filePath;
    QLabel* fileNameLabel = nullptr;
    QLabel* filePathLabel = nullptr;
    QPushButton* openFolderBtn = nullptr;
    QPushButton* openFileBtn = nullptr;
};

class TranslateWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TranslateWidget(QWidget *parent = nullptr);
    ~TranslateWidget() override;

    struct Config {
        QString audioPath_;
        QString modelName_;
        QString outputMode_;
        int language_;

        Config(const QString& audioPath = QString(),
               const QString& modelName = QStringLiteral("ggml-tiny.bin"),
               const QString& outputMode = QStringLiteral("txt"),
               int language = 0)
            : audioPath_(audioPath)
            , modelName_(modelName)
            , outputMode_(outputMode)
            , language_(language)
        {}
    };

    void setPluginHostContext(QObject* hostContext, const QStringList& grantedPermissions);

    void on_transcribeButton_clicked();
    void showTipMessage(const QString &msg);
    void addResultItem(const QString &fileName, const QString &filePath);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void buildUi();
    void bindSignals();
    void resetUiStateAfterTask();

private:
    QObject* m_hostContext = nullptr;
    QStringList m_grantedPermissions;

    QLabel* subtitleLabel = nullptr;

    QLabel* fileLabel = nullptr;
    QLineEdit* filePathEdit = nullptr;
    QPushButton* browseButton = nullptr;

    QLabel* modelLabel = nullptr;
    QComboBox* modelCombo = nullptr;

    QLabel* formatLabel = nullptr;
    QComboBox* formatCombo = nullptr;

    QLabel* langLabel = nullptr;
    QComboBox* langCombo = nullptr;

    QPushButton* transcribeButton = nullptr;
    QTextEdit* resultEdit = nullptr;

    QLabel* resultListLabel = nullptr;
    QListWidget* resultList = nullptr;

    QProgressBar* progressBar = nullptr;
    QVBoxLayout* mainLayout = nullptr;

    QVector<float> pcmf32_;
    Config config_;

    const QString modelPath = QStringLiteral("E:/FFmpeg_whisper/ggml-models/");

    std::mutex mtx;
    std::condition_variable cv;

    std::shared_ptr<TakePcm> take_pcm;
    QThread* decodeThread_ = nullptr;

    std::atomic<bool> translating{false};
    std::atomic<bool> pcmReady{false};
    transformFactory factory;

    QMap<int, QString> languageMap {
        {0, QStringLiteral("zh")},
        {1, QStringLiteral("en")},
        {2, QStringLiteral("ja")},
        {3, QStringLiteral("ko")},
        {4, QStringLiteral("fr")},
        {5, QStringLiteral("de")},
        {6, QStringLiteral("es")}
    };

    QMap<QString, const char*> initialPromptMap {
        {QStringLiteral("zh"), "以下是普通话语音，请输出简体中文文本："},
        {QStringLiteral("en"), "The following audio is in English. Please output English text:"},
        {QStringLiteral("ja"), "以下は日本語音声です。日本語テキストで出力してください："},
        {QStringLiteral("ko"), "다음은 한국어 음성입니다. 한국어 텍스트로 출력하세요:"},
        {QStringLiteral("fr"), "Le contenu suivant est en francais, veuillez produire du texte francais :"},
        {QStringLiteral("de"), "Der folgende Inhalt ist auf Deutsch, bitte geben Sie deutschen Text aus:"},
        {QStringLiteral("es"), "El siguiente contenido esta en espanol, genere texto en espanol:"}
    };

signals:
    void signal_begin_tranform();
    void signal_erorr(const QString&);
    void signal_begin_take_pcm(QString path);
    void signal_outFile(const QStringList &);

public slots:
    void on_signal_begin_transform();
    void on_signal_send_data(uint8_t *buffer, int bufferSize, qint64 timeMap);
    void on_signal_decodeEnd();
    void on_signal_outFile(const QStringList &);
    void updateProgress(int);
};

#endif // TRANSLATEWIDGET_H
