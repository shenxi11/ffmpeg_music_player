#include "lrc_analyze.h"

LrcAnalyze::LrcAnalyze()
{
    connect(this, &LrcAnalyze::begin_take_lrc, this, &LrcAnalyze::take_lrc);

    connect(this, &LrcAnalyze::Begin_send, this, &LrcAnalyze::begin_send);
}

 LrcAnalyze::~LrcAnalyze()
{
    qDebug()<<"Destruct lrc_analyze";
}

//void lrc_analyze::run()
//{



//}
// 使用 uchardet 检测文件编码
QString LrcAnalyze::detectEncodingWithUchardet(const QString &filePath)
{
    QProcess process;
    process.start("uchardet", QStringList() << filePath);
    process.waitForFinished();

    QString encoding = process.readAllStandardOutput().trimmed();
    return encoding;
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
        qDebug() << "检测到的编码为:" << encoding;

        // 如果编码不是 UTF-8，读取并转换为 UTF-8
        if (encoding != "UTF-8")
        {
            QString content = readFileWithEncoding(QString::fromStdString(lrcFile), encoding.toUtf8());

            if (!content.isEmpty())
            {
                // 覆盖原文件并保存为 UTF-8 编码
                saveFileAsUtf8(content, QString::fromStdString(lrcFile));
                qDebug() << "文件已成功转换为 UTF-8 并覆盖原文件";
            }
            else
            {
                qWarning() << "无法读取文件内容";
            }
        }
        else
        {
            qDebug() << "文件已经是 UTF-8 编码，不需要转换";
        }

        emit Begin_send(lrcFile);

    }
    else
    {

    }



    //    // 打印解析后的歌词
    //    for (const auto& [time, text] : lyrics) {
    //         qDebug() << "Time:" << time << "ms," << "Lyrics:" << QString::fromStdString(text);
    //    }
}
void LrcAnalyze::begin_send(std::string lrcFile)
{
    lyrics.clear();

    //removeEmptyLinesWithTimestamps(lrcFile);

    lyrics = parseLrcFile(lrcFile);  // 解析歌词文件

   // clearEmptyLines(lyrics);

    emit send_lrc(lyrics);
}
// 函数用于检查字符串是否为空或仅包含空白字符
bool LrcAnalyze::isBlank(const std::string &str)
{
    return std::all_of(str.begin(), str.end(), ::isspace);
}

// 函数用于清除歌词中的空行
void LrcAnalyze::clearEmptyLines(std::map<int, std::string> &lyrics)
{
    // 使用标准库算法从 map 中移除空行
    for (auto it = lyrics.begin(); it != lyrics.end(); )
    {
        if (isBlank(it->second))
        {
            it = lyrics.erase(it); // 移除空行
        }
        else
        {
            ++it; // 继续遍历
        }
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

std::map<int, std::string> LrcAnalyze::parseLrcFile(const std::string& lrcFile)
{
    std::ifstream file(lrcFile);
    std::map<int, std::string> lyrics;

    if (!file.is_open())
    {
        throw std::runtime_error("Could not open file: " + lrcFile);
    }

    std::string line;
    std::regex regexPattern(R"(\[(\d{2}:\d{2}\.\d{2})\](.*))");  // 正则匹配 [mm:ss.xx] 和歌词内容
    std::smatch match;

    while (std::getline(file, line))
    {
        if (std::regex_match(line, match, regexPattern))
        {
            std::string timeStr = match[1];  // 获取时间戳
            std::string lyricText = match[2];  // 获取歌词文本

            // 检查歌词内容是否为空
            if (!lyricText.empty() && lyricText.find_first_not_of(" \t\r\n") != std::string::npos)
            {
                int timeInMs = timeToMilliseconds(timeStr);  // 将时间戳转换为毫秒
                lyrics[timeInMs] = lyricText;  // 存储时间和歌词

            }
        }
    }

    file.close();
    return lyrics;
}
bool LrcAnalyze::hasTimestamp(const std::string& line)
{
    return line.size() > 1 && line[0] == '[' && line[1] >= '0' && line[1] <= '9';
}

// 函数用于清除歌词文件中的空行
void LrcAnalyze::removeEmptyLinesWithTimestamps(const std::string& filename)
{
    std::ifstream fileIn(filename);
    if (!fileIn.is_open())
    {
        std::cerr << "Error opening file for reading: " << filename << std::endl;
        return;
    }

    std::vector<std::string> lines;
    std::string line;

    // 读取文件内容到内存中
    while (std::getline(fileIn, line))
    {
        if (!(hasTimestamp(line) && line.find_first_not_of(" \t\r\n") == line.size()))
        {
            lines.push_back(line); // 只保留需要的行
        }
    }
    fileIn.close();

    std::ofstream fileOut(filename);
    if (!fileOut.is_open())
    {
        std::cerr << "Error opening file for writing: " << filename << std::endl;
        return;
    }

    // 将处理后的内容写回到文件中
    for (const auto& l : lines)
    {
        fileOut << l << '\n';
    }
    fileOut.close();
}

