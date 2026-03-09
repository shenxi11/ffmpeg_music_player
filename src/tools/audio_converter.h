#ifndef AUDIOCONVERTER_H
#define AUDIOCONVERTER_H

#include <QWidget>
#include <QCloseEvent>
#include <QComboBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QStringList>
#include <QTableWidget>
#include <QTime>

class AudioConverter : public QWidget
{
    Q_OBJECT
public:
    explicit AudioConverter(QWidget *parent = nullptr);

    // 由插件层注入主程序上下文与授权权限，便于插件在同一架构下运行。
    void setPluginHostContext(QObject* hostContext, const QStringList& grantedPermissions);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onAddAudioClicked();
    void onImportCDClicked();
    void onClearCompletedClicked();
    void onChangeDirClicked();
    void onOpenFolderClicked();
    void onStartConversionClicked();
    void onFormatChanged();
    void onEncoderChanged();
    void onBitrateChanged();
    void onConversionFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void updateProgress();

private:
    void setupUI();
    void setupTable();
    void addAudioFile(const QString &filePath);
    void startNextConversion();
    QString getOutputPath(const QString &inputPath);
    QTime getAudioDuration(const QString &filePath);

    QString createButtonStyle();
    QString createComboStyle();
    QString createTableStyle();

private:
    QObject* m_hostContext = nullptr;
    QStringList m_grantedPermissions;

    QTableWidget* fileTable = nullptr;
    QComboBox* formatCombo = nullptr;
    QComboBox* encoderCombo = nullptr;
    QComboBox* bitrateCombo = nullptr;
    QLineEdit* outputDirEdit = nullptr;
    QPushButton* addAudioBtn = nullptr;
    QPushButton* importCDBtn = nullptr;
    QPushButton* clearCompletedBtn = nullptr;
    QPushButton* changeDirBtn = nullptr;
    QPushButton* openFolderBtn = nullptr;
    QPushButton* startConversionBtn = nullptr;
    QProgressBar* progressBar = nullptr;
    QLabel* statusLabel = nullptr;

    QProcess* ffmpegProcess = nullptr;
    QStringList conversionQueue;
    int currentConversionIndex = -1;
    QString ffmpegPath;
    QString outputDirectory;
};

#endif // AUDIOCONVERTER_H
