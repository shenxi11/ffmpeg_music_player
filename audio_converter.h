#ifndef AUDIOCONVERTER_H
#define AUDIOCONVERTER_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QProcess>
#include <QProgressBar>
#include <QTimer>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QDesktopServices>
#include <QTime>
#include <QCloseEvent>
#include "headers.h"

class AudioConverter : public QWidget
{
    Q_OBJECT
public:
    explicit AudioConverter(QWidget *parent = nullptr);

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
    
    // UI 组件
    QTableWidget* fileTable;
    QComboBox* formatCombo;
    QComboBox* encoderCombo;
    QComboBox* bitrateCombo;
    QLineEdit* outputDirEdit;
    QPushButton* addAudioBtn;
    QPushButton* importCDBtn;
    QPushButton* clearCompletedBtn;
    QPushButton* changeDirBtn;
    QPushButton* openFolderBtn;
    QPushButton* startConversionBtn;
    QProgressBar* progressBar;
    QLabel* statusLabel;
    
    // 转换相关
    QProcess* ffmpegProcess;
    QStringList conversionQueue;
    int currentConversionIndex;
    QString ffmpegPath;
    QString outputDirectory;
    
    QString createButtonStyle();
    QString createComboStyle();
    QString createTableStyle();
};

#endif // AUDIOCONVERTER_H
