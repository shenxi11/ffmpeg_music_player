#ifndef DOWNLOADTHREAD_H
#define DOWNLOADTHREAD_H

#include <QObject>
#include "headers.h"

class DownloadThread: public QRunnable
{
public:
    DownloadThread();
private:
    int taskId;
};

#endif // DOWNLOADTHREAD_H
