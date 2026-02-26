#ifndef LOGGER_H
#define LOGGER_H

#include <QString>

// Initialize logger. If path is empty, logger will use a default path.
void initLogger(const QString& logFilePath = QString());

// Shutdown logger and flush pending logs.
void cleanupLogger();

// Current log file path after initialization.
QString currentLogFilePath();

#endif // LOGGER_H
