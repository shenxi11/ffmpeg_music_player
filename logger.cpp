#include "logger.h"
#include <QDateTime>
#include <QProcessEnvironment>
#include <QFile>
#include <QMutex>
#include <QStringList>
#include <QTextStream>
#include <QtDebug>
#include <Windows.h>
#include <cstdlib>

static QFile* g_logFile = nullptr;
static QTextStream* g_logStream = nullptr;
static QMutex g_logMutex;

void customMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Q_UNUSED(context);
    QMutexLocker locker(&g_logMutex);

    if (msg.contains("pa_stream_begin_write")) {
        return;
    }

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

    const QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString normalized = msg;
    normalized.replace("\r\n", "\n");
    normalized.replace('\r', '\n');
    const QStringList lines = normalized.split('\n');

    for (const QString& line : lines) {
        const QString logMessage = QString("[%1] [%2] %3").arg(timestamp, level, line);

        if (g_logStream) {
            (*g_logStream) << logMessage << Qt::endl;
            g_logStream->flush();
        }

        const bool logToStderr = QProcessEnvironment::systemEnvironment().contains("LOG_TO_STDERR");
        if (logToStderr) {
            QTextStream(stderr) << logMessage << Qt::endl;
        }

#ifdef Q_OS_WIN
        OutputDebugStringW(reinterpret_cast<const wchar_t*>(logMessage.utf16()));
        OutputDebugStringW(L"\n");
#else
        if (!logToStderr) {
            QTextStream(stderr) << logMessage << Qt::endl;
        }
#endif
    }

    if (type == QtFatalMsg) {
        abort();
    }
}

void initLogger(const QString& logFilePath)
{
    g_logFile = new QFile(logFilePath);
    if (g_logFile->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        g_logStream = new QTextStream(g_logFile);
        g_logStream->setCodec("UTF-8");
        qInstallMessageHandler(customMessageHandler);
        qDebug() << "Logger initialized. Log file:" << logFilePath;
    } else {
        qWarning() << "Failed to open log file:" << logFilePath << "Error:" << g_logFile->errorString();
    }
}

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
