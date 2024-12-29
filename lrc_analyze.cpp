#include "lrc_analyze.h"
#include "httprequest.h"
LrcAnalyze::LrcAnalyze()
{
    connect(this, &LrcAnalyze::begin_take_lrc, this, &LrcAnalyze::take_lrc);

    connect(this, &LrcAnalyze::Begin_send, this, &LrcAnalyze::begin_send);
}

LrcAnalyze::~LrcAnalyze()
{
    //qDebug()<<"Destruct lrc_analyze";
}

//void lrc_analyze::run()
//{



//}
//检测文件编码
QString LrcAnalyze::detectEncodingWithUchardet(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open file:" << file.errorString();
        return QString();
    }

    QByteArray fileData = file.readAll();
    file.close();

    // 尝试不同的编码
    QList<QByteArray> codecs = QTextCodec::availableCodecs();
    for (const QByteArray &codecName : codecs) {
        QTextCodec *codec = QTextCodec::codecForName(codecName);
        if (codec && codec->canEncode(fileData)) {
            return QString(codecName);
        }
    }
    return "";
}

// 根据指定的编码格式读取文件
QString LrcAnalyze::readFileWithEncoding(const QString &filePath, const QByteArray &encoding)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "无法打开文件:" << filePath;
        return "";
    }

    QTextStream in(&file);
    QTextCodec *codec = QTextCodec::codecForName(encoding);

    if (!codec)
    {
        qWarning() << "编码格式无效:" << encoding;
        return "";
    }

    in.setCodec(codec);
    QString content = in.readAll();

    file.close();
    return content;
}

// 将内容覆盖原文件，并保存为 UTF-8 编码
void LrcAnalyze::saveFileAsUtf8(const QString &content, const QString &filePath)
{
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        qWarning() << "无法打开文件进行写入:" << filePath;
        return;
    }

    QTextStream out(&file);
    // 设置输出流的编码为 UTF-8
    out.setCodec("UTF-8");
    out << content;
    file.close();
}
void LrcAnalyze::take_lrc(QString Path)
{
    std::string lrcFile = Path.toStdString();  // 歌词文件路径

    std::reverse(lrcFile.begin(),lrcFile.end());

    int pos = lrcFile.find(".");
    int len = lrcFile.size();

    std::reverse(lrcFile.begin(), lrcFile.end());
    lrcFile = lrcFile.substr(0,len-pos)+"lrc";

    QString encoding = detectEncodingWithUchardet(QString::fromStdString(lrcFile));

    if (!encoding.isEmpty())
    {
        if (encoding != "UTF-8")
        {
            QString content = readFileWithEncoding(QString::fromStdString(lrcFile), encoding.toUtf8());

            if (!content.isEmpty())
            {
                saveFileAsUtf8(content, QString::fromStdString(lrcFile));
            }
            else
            {
                qWarning() << "无法读取文件内容";
            }
        }
    }
    emit Begin_send(QString::fromStdString(lrcFile));
}
void LrcAnalyze::begin_send(QString lrcFile)
{

    lyrics.clear();
    if(!lrcFile.contains("http"))
    {
        lyrics = parseLrcFile(lrcFile);  // 解析歌词文件
        emit send_lrc(lyrics);
    }
    else
    {
        parseLrcFileFromUrl(lrcFile);
    }
}
// 将时间戳 [mm:ss.xx] 转换为毫秒
int LrcAnalyze::timeToMilliseconds(const std::string& timeStr)
{
    int minutes = std::stoi(timeStr.substr(0, 2));
    int seconds = std::stoi(timeStr.substr(3, 2));
    int milliseconds = std::stoi(timeStr.substr(6, 2)) * 10;
    return (minutes * 60 + seconds) * 1000 + milliseconds;
}

std::map<int, std::string> LrcAnalyze::parseLrcFile(const QString& lrcFile)
{
    QFile file(lrcFile);
    std::map<int, std::string> lyrics;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        throw std::runtime_error("Could not open file: " + lrcFile.toStdString());
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");
    QString line;
    QRegularExpression regexPattern(R"(\[(\d{2}:\d{2}\.\d{2})\](.*))");  // 正则匹配 [mm:ss.xx] 和歌词内容

    while (in.readLineInto(&line))
    {
        QRegularExpressionMatch match = regexPattern.match(line);
        if (match.hasMatch())
        {
            QString timeStr = match.captured(1);  // 获取时间戳
            QString lyricText = match.captured(2);  // 获取歌词文本

            // 检查歌词内容是否为空
            if (!lyricText.trimmed().isEmpty())
            {
                int timeInMs = timeToMilliseconds(timeStr.toStdString());  // 转换为 std::string
                lyrics[timeInMs] = lyricText.toStdString();  // 转换为 std::string 并存储
            }
        }
    }

    file.close();
    return lyrics;
}
void LrcAnalyze::parseLrcFileFromUrl(const QString& urlString)
{
    auto request = HttpRequest::getInstance();
    request->get_file(urlString);

    connect(request, &HttpRequest::signal_lrc, this, [=](QStringList arg){

        std::map<int, std::string> lyrics;
        QRegularExpression regexPattern(R"(\[(\d{2}:\d{2}\.\d{2})\](.*))");  // 正则匹配 [mm:ss.xx] 和歌词内容
        QStringList lines = arg;

        for (const QString& line : lines)
        {
            QRegularExpressionMatch match = regexPattern.match(line);
            if (match.hasMatch())
            {
                QString timeStr = match.captured(1);  // 获取时间戳
                QString lyricText = match.captured(2);  // 获取歌词文本

                // 检查歌词内容是否为空
                if (!lyricText.trimmed().isEmpty())
                {
                    int timeInMs = timeToMilliseconds(timeStr.toStdString());  // 转换为 std::string
                    lyrics[timeInMs] = lyricText.toStdString();  // 转换为 std::string 并存储
                }
            }
        }
        this->lyrics = lyrics;
        emit this->send_lrc(this->lyrics);
        disconnect(request, &HttpRequest::signal_lrc, nullptr, nullptr);
    });

}



