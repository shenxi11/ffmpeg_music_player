#ifndef HEADERS_H
#define HEADERS_H

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavfilter/avfilter.h>
#include <libavdevice/avdevice.h>
#include <libswresample/swresample.h>
}
#include <locale.h>
#include <QMediaPlayer>
#include <QPushButton>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QFile>
#include <QTimer>
#include <QDebug>
#include <QDir>
#include <QThread>
#include <memory>
#include <QFileDialog>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <regex>
#include <string>
#include <map>
#include <QProcess>
#include <QTextCodec>
#include <QThreadPool>
#include <mutex>
#include <QWaitCondition>
#include <QMetaObject>
#include <QMutex>
#include <QQueue>
#include <QRegularExpression>
#include <QWaitCondition>
#include <QByteArray>
#include <QVBoxLayout>
#include <QPainter>
#include <QRegion>
#include <QLabel>



#define BUFFER_SIZE 17640
#define AUDIO_BUFFER_SIZE 65536
#define RATE 44100
#define CHANNELS 2
#define SAMPLE_SIZE 16
#define MAX_BUFFER_SIZE 100
#include <QLoggingCategory>




#endif // HEADERS_H
