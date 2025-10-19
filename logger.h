#ifndef LOGGER_H
#define LOGGER_H

#include <QString>

// 初始化日志系统
void initLogger(const QString& logFilePath);

// 清理日志系统
void cleanupLogger();

#endif // LOGGER_H
