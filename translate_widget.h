
#ifndef TRANSLATEWIDGET_H
#define TRANSLATEWIDGET_H


#include <QObject>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QFileDialog>
#include <QComboBox>
#include <QProgressBar>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <QString>

#include "whisper.h"
#include "take_pcm.h"
void saveTXT(struct whisper_context* ctx, QStringList &outputLines);
void saveJSON(struct whisper_context* ctx, QStringList &outputLines);
void saveKAR(struct whisper_context* ctx, QStringList &outputLines);
void saveLRC(struct whisper_context* ctx, QStringList &outputLines);
void saveVTT(struct whisper_context* ctx, QStringList &outputLines);
void saveSRT(struct whisper_context* ctx, QStringList &outputLines);

class transformFactory{
public:
    transformFactory(){
        funcMp["txt"] = saveTXT;
        funcMp["json"] = saveJSON;
        funcMp["vtt"] = saveVTT;
        funcMp["srt"] = saveSRT;
        funcMp["lrc"] = saveLRC;
        funcMp["kar"] = saveKAR;
    }
   void save(const QString &name, struct whisper_context* ctx, QStringList &outputLines);
private:
   std::map<const QString, std::function<void(struct whisper_context* ctx, QStringList &outputLines)>> funcMp;
};

// 结果列表项Widget
class ResultListItem : public QWidget
{
    Q_OBJECT
public:
    explicit ResultListItem(const QString &fileName, const QString &filePath, QWidget *parent = nullptr);
    QString getFileName() const { return fileName; }
    QString getFilePath() const { return filePath; }

private slots:
    void onOpenFolderClicked();
    void onOpenFileClicked();

private:
    QString fileName;
    QString filePath;
    QLabel* fileNameLabel;
    QLabel* filePathLabel;
    QPushButton* openFolderBtn;
    QPushButton* openFileBtn;
};

class TranslateWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TranslateWidget(QWidget *parent = nullptr);

    struct Config{
        QString audioPath_;
        QString modelName_;
        QString outputMode_;
        int language_;

        Config(QString audioPath = "", QString modelName = "ggml-tiny.bin", QString outputMode = "txt", int language = 0):
        audioPath_(audioPath), modelName_(modelName),outputMode_(outputMode),language_(0){};
    };

    void on_transcribeButton_clicked();
    void showTipMessage(const QString &msg);
    void receive_data();
    void addResultItem(const QString &fileName, const QString &filePath);
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;


private:
    QLabel* titleLabel;
    QLabel* fileLabel;
    QLineEdit* filePathEdit;
    QPushButton* browseButton;
    QLabel* modelLabel;
    QComboBox* modelCombo;
    QLabel* formatLabel;
    QComboBox* formatCombo;
    QLabel* langLabel;
    QComboBox* langCombo;
    QPushButton* transcribeButton;
    QTextEdit* resultEdit;
    QLabel* resultListLabel;
    QListWidget* resultList;
    QProgressBar* progressBar;
    QVBoxLayout* mainLayout;
    QHBoxLayout* fileLayout;
    QHBoxLayout* modelLayout;
    QHBoxLayout* formatLayout;
    QHBoxLayout* langLayout;
    QHBoxLayout* titleBarLayout;
    QWidget* titleBarWidget;
    QPushButton* minimizeButton;
    QPushButton* maximizeButton;
    QPushButton* closeButton;

    bool mousePressed = false;
    QPoint mouseStartPoint;
    QPoint windowStartPoint;

    QVector<float> pcmf32_;

    Config config_;
    const QString modelPath = "E:/FFmpeg_whisper/ggml-models/";

    std::mutex mtx;
    std::condition_variable cv;

    std::shared_ptr<TakePcm> take_pcm;

    std::atomic<bool> translating = false;
    transformFactory factory;

    QMap<int, QString> languageMap = {
        {0, "zh"},
        {1, "en"},
        {2, "ja"},
        {3, "ko"},
        {4, "fr"},
        {5, "de"},
        {6, "es"}
    };
    QMap<QString, const char*> initialPromptMap = {

    {"zh", "以下是普通话的句子，使用简体中文："},

    {"en", "The following is an English sentence, use English:"},

    {"ja", "以下は日本語の文です、日本語を使用してください："},

    {"ko", "다음은 한국어 문장입니다. 한국어를 사용하세요:"},

    {"fr", "Ce qui suit est une phrase en français, utilisez le français :"},

    {"de", "Folgendes ist ein deutscher Satz, verwenden Sie Deutsch:"},

    {"es", "Lo siguiente es una oración en español, use español:"}

    };
signals:
    void signal_wav_transfromed(int exit_code);
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
