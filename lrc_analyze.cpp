#include "lrc_analyze.h"

lrc_analyze::lrc_analyze()
{

}
void lrc_analyze::run(){
    connect(this,&lrc_analyze::begin_take_lrc,this,&lrc_analyze::take_lrc);

    connect(this,&lrc_analyze::Begin_send,this,&lrc_analyze::begin_send);


}
// 使用 uchardet 检测文件编码
QString lrc_analyze::detectEncodingWithUchardet(const QString &filePath) {
    QProcess process;
    process.start("uchardet", QStringList() << filePath);
    process.waitForFinished();

    QString encoding = process.readAllStandardOutput().trimmed();
    return encoding;
}

// 根据指定的编码格式读取文件
QString lrc_analyze::readFileWithEncoding(const QString &filePath, const QByteArray &encoding) {
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "无法打开文件:" << filePath;
        return "";
    }

    QTextStream in(&file);
    QTextCodec *codec = QTextCodec::codecForName(encoding);

    if (!codec) {
        qWarning() << "编码格式无效:" << encoding;
        return "";
    }

    in.setCodec(codec);
    QString content = in.readAll();

    file.close();
    return content;
}

// 将内容覆盖原文件，并保存为 UTF-8 编码
void lrc_analyze::saveFileAsUtf8(const QString &content, const QString &filePath) {
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning() << "无法打开文件进行写入:" << filePath;
        return;
    }

    QTextStream out(&file);
    // 设置输出流的编码为 UTF-8
    out.setCodec("UTF-8");
    out << content;
    file.close();
}
void lrc_analyze::take_lrc(QString Path){
    std::string lrcFile = Path.toStdString();  // 歌词文件路径


    std::reverse(lrcFile.begin(),lrcFile.end());



    int pos=lrcFile.find(".");
    int len=lrcFile.size();

    std::reverse(lrcFile.begin(),lrcFile.end());

    lrcFile=lrcFile.substr(0,len-pos)+"lrc";

    QString encoding = detectEncodingWithUchardet(QString::fromStdString(lrcFile));
    if (!encoding.isEmpty()) {
        qDebug() << "检测到的编码为:" << encoding;

        // 如果编码不是 UTF-8，读取并转换为 UTF-8
        if (encoding != "UTF-8") {
            QString content = readFileWithEncoding(QString::fromStdString(lrcFile), encoding.toUtf8());

            if (!content.isEmpty()) {
                // 覆盖原文件并保存为 UTF-8 编码
                saveFileAsUtf8(content, QString::fromStdString(lrcFile));
                qDebug() << "文件已成功转换为 UTF-8 并覆盖原文件";
            } else {
                qWarning() << "无法读取文件内容";
            }
        } else {
            qDebug() << "文件已经是 UTF-8 编码，不需要转换";
        }
    } else {
        qWarning() << "无法检测文件编码";
    }

    emit Begin_send(lrcFile);

    //    // 打印解析后的歌词
    //    for (const auto& [time, text] : lyrics) {
    //         qDebug() << "Time:" << time << "ms," << "Lyrics:" << QString::fromStdString(text);
    //    }
}
void lrc_analyze::begin_send(std::string lrcFile){
    lyrics.clear();

    lyrics = parseLrcFile(lrcFile);  // 解析歌词文件

    emit send_lrc(lyrics);
}

// 将时间戳 [mm:ss.xx] 转换为毫秒
int lrc_analyze::timeToMilliseconds(const std::string& timeStr) {
    int minutes = std::stoi(timeStr.substr(0, 2));
    int seconds = std::stoi(timeStr.substr(3, 2));
    int milliseconds = std::stoi(timeStr.substr(6, 2)) * 10;
    return (minutes * 60 + seconds) * 1000 + milliseconds;
}
// 解析 .lrc 文件
std::map<int, std::string> lrc_analyze::parseLrcFile(const std::string& lrcFile) {
    std::ifstream file(lrcFile);

    std::string line;
    std::regex regexPattern(R"(\[(\d{2}:\d{2}\.\d{2})\](.*))");  // 正则匹配 [mm:ss.xx] 和歌词内容
    std::smatch match;

    while (std::getline(file, line)) {
        if (std::regex_match(line, match, regexPattern)) {
            std::string timeStr = match[1];  // 获取时间戳
            std::string lyricText = match[2];  // 获取歌词文本
            int timeInMs = timeToMilliseconds(timeStr);  // 将时间戳转换为毫秒
            lyrics[timeInMs] = lyricText;  // 存储时间和歌词
        }
    }
    return lyrics;
}

