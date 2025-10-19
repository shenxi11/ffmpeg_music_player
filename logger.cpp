#include "logger.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QtDebug>
#include <Windows.h>  // For OutputDebugStringW

// 全局日志文件
static QFile* g_logFile = nullptr;
static QTextStream* g_logStream = nullptr;
static QMutex g_logMutex;

// 自定义消息处理函数
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QMutexLocker locker(&g_logMutex);
    
    QString level;
    switch (type) {
        case QtDebugMsg:
            level = "DEBUG";
            break;
        case QtInfoMsg:
            level = "INFO";
            break;
        case QtWarningMsg:
            level = "WARNING";
            break;
        case QtCriticalMsg:
            level = "CRITICAL";
            break;
        case QtFatalMsg:
            level = "FATAL";
            break;
    }
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString logMessage = QString("[%1] [%2] %3").arg(timestamp, level, msg);
    
    // 写入文件
    if (g_logStream) {
        (*g_logStream) << logMessage << Qt::endl;
        g_logStream->flush();
    }
    
    // 同时输出到调试器（VS 输出窗口）
#ifdef Q_OS_WIN
    OutputDebugStringW(reinterpret_cast<const wchar_t*>(logMessage.utf16()));
    OutputDebugStringW(L"\n");
#endif
}

// 初始化日志系统
void initLogger(const QString& logFilePath)
{
    g_logFile = new QFile(logFilePath);
    if (g_logFile->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        g_logStream = new QTextStream(g_logFile);
        g_logStream->setCodec("UTF-8");
        qInstallMessageHandler(customMessageHandler);
        
        // 注意：此时的 qDebug 已经会通过 customMessageHandler 输出
        qDebug() << "Logger initialized. Log file:" << logFilePath;
    } else {
        // 如果文件打开失败，直接输出到控制台（不通过文件）
        qWarning() << "Failed to open log file:" << logFilePath << "Error:" << g_logFile->errorString();
    }
}

// 清理日志系统
void cleanupLogger()
{
    qInstallMessageHandler(nullptr);
    
    if (g_logStream) {
        delete g_logStream;
        g_logStream = nullptr;
    }
    
    if (g_logFile) {
        g_logFile->close();
        delete g_logFile;
        g_logFile = nullptr;
    }
}
