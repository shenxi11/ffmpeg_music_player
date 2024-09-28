#ifndef LRC_ANALYZE_H
#define LRC_ANALYZE_H

#include <QObject>
#include"headers.h"
class LrcAnalyze : public QObject
{
    Q_OBJECT
public:
    explicit LrcAnalyze();

    ~LrcAnalyze();
private slots:
    void begin_send(std::string lrcFile);

private:
    bool hasTimestamp(const std::string& line);

    void removeEmptyLinesWithTimestamps(const std::string& filename);

    bool isBlank(const std::string &str) ;

    void clearEmptyLines(std::map<int, std::string> &lyrics);

    void take_lrc(QString Path);

    int timeToMilliseconds(const std::string& timeStr);

    std::map<int, std::string> parseLrcFile(const std::string& lrcFile);

    QString detectEncodingWithUchardet(const QString &filePath);

    void saveFileAsUtf8(const QString &content, const QString &outputFilePath);

    QString readFileWithEncoding(const QString &filePath, const QByteArray &encoding);

//    void run() override;

    std::map<int, std::string> lyrics;  // 用来存储解析的歌词，时间戳为 key，歌词文本为 value

signals:
    void begin_take_lrc(QString Path);

    void send_lrc(std::map<int, std::string> lyrics);

    void Begin_send(std::string lrcFile);
};

#endif // LRC_ANALYZE_H
