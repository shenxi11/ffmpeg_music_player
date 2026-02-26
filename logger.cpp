#include "logger.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QProcessEnvironment>
#include <QStringList>
#include <QTextStream>
#include <QtDebug>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>

namespace {

class AsyncLogWriter {
public:
    AsyncLogWriter() = default;
    ~AsyncLogWriter() { stop(); }

    bool start(const QString& path)
    {
        m_filePath = QDir::cleanPath(path);
        m_file.setFileName(m_filePath);
        if (!m_file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            return false;
        }

        m_stream.setDevice(&m_file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        m_stream.setCodec("UTF-8");
#endif
        m_stream.setGenerateByteOrderMark(true);

        m_running.store(true, std::memory_order_release);
        m_thread = std::thread([this]() { run(); });
        return true;
    }

    void stop()
    {
        bool expected = true;
        if (!m_running.compare_exchange_strong(expected, false, std::memory_order_acq_rel)) {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
        }
        m_cv.notify_all();

        if (m_thread.joinable()) {
            m_thread.join();
        }

        m_stream.flush();
        m_file.close();
    }

    void enqueue(const QString& line)
    {
        if (!m_running.load(std::memory_order_acquire)) {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push_back(line);
        }
        m_cv.notify_one();
    }

    QString filePath() const
    {
        return m_filePath;
    }

private:
    void run()
    {
        for (;;) {
            std::deque<QString> batch;

            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [this]() {
                    return !m_running.load(std::memory_order_acquire) || !m_queue.empty();
                });

                if (m_queue.empty() && !m_running.load(std::memory_order_acquire)) {
                    break;
                }

                batch.swap(m_queue);
            }

            for (const QString& line : batch) {
                m_stream << line << '\n';
            }
            m_stream.flush();
        }
    }

private:
    QFile m_file;
    QTextStream m_stream;
    QString m_filePath;

    std::atomic<bool> m_running{false};
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<QString> m_queue;
};

QMutex g_loggerMutex;
std::unique_ptr<AsyncLogWriter> g_asyncWriter;
QString g_activeLogPath;
bool g_logToStderr = false;

QString resolveLogPath(const QString& requestedPath)
{
    const QString explicitPath = requestedPath.trimmed();
    if (!explicitPath.isEmpty()) {
        return QDir::cleanPath(explicitPath);
    }

    const QString envPath = QString::fromLocal8Bit(qgetenv("PRINT_LOG_PATH")).trimmed();
    if (!envPath.isEmpty()) {
        return QDir::cleanPath(QDir::fromNativeSeparators(envPath));
    }

    return QDir::current().absoluteFilePath(QStringLiteral("打印日志.txt"));
}

void writeDebugOutput(const QString& text)
{
#ifdef Q_OS_WIN
    OutputDebugStringW(reinterpret_cast<const wchar_t*>(text.utf16()));
    OutputDebugStringW(L"\n");
#else
    QTextStream(stderr) << text << Qt::endl;
#endif
}

} // namespace

void customMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Q_UNUSED(context);

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

    QStringList lines = normalized.split('\n', Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        lines << QString();
    }

    QMutexLocker locker(&g_loggerMutex);
    for (const QString& line : lines) {
        const QString logMessage = QString("[%1] [%2] %3").arg(timestamp, level, line);

        if (g_asyncWriter) {
            g_asyncWriter->enqueue(logMessage);
        }

        if (g_logToStderr) {
            QTextStream(stderr) << logMessage << Qt::endl;
        }

#ifdef Q_OS_WIN
        writeDebugOutput(logMessage);
#else
        if (!g_logToStderr) {
            writeDebugOutput(logMessage);
        }
#endif
    }

    if (type == QtFatalMsg) {
        std::abort();
    }
}

void initLogger(const QString& logFilePath)
{
    QString initializedPath;

    {
        QMutexLocker locker(&g_loggerMutex);

        if (g_asyncWriter) {
            qInstallMessageHandler(nullptr);
            g_asyncWriter->stop();
            g_asyncWriter.reset();
            g_activeLogPath.clear();
        }

        g_logToStderr = QProcessEnvironment::systemEnvironment().contains("LOG_TO_STDERR");
        g_activeLogPath = resolveLogPath(logFilePath);

        g_asyncWriter = std::make_unique<AsyncLogWriter>();
        if (!g_asyncWriter->start(g_activeLogPath)) {
            const QString errorMessage = QString("[LOGGER] Failed to open log file: %1").arg(g_activeLogPath);
            writeDebugOutput(errorMessage);
            g_asyncWriter.reset();
            return;
        }

        qInstallMessageHandler(customMessageHandler);
        initializedPath = g_activeLogPath;
    }

    qDebug() << "Logger initialized. Log file:" << initializedPath;
}

void cleanupLogger()
{
    QMutexLocker locker(&g_loggerMutex);

    qInstallMessageHandler(nullptr);

    if (g_asyncWriter) {
        g_asyncWriter->stop();
        g_asyncWriter.reset();
    }

    g_activeLogPath.clear();
}

QString currentLogFilePath()
{
    QMutexLocker locker(&g_loggerMutex);
    return g_activeLogPath;
}
