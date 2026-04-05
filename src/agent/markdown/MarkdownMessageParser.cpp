#include "MarkdownMessageParser.h"

#include <QStringList>
#include <QVector>

QVariantList MarkdownMessageParser::parse(const QString& rawText) const
{
    QVariantList blocks;

    const QStringList lines = rawText.split(QChar('\n'), Qt::KeepEmptyParts);

    bool inCodeBlock = false;
    QString codeLanguage;
    QStringList codeLines;
    QStringList paragraphLines;
    QStringList listLines;

    auto flushParagraph = [&]() {
        if (paragraphLines.isEmpty()) {
            return;
        }
        const QString text = paragraphLines.join(QStringLiteral("\n")).trimmed();
        paragraphLines.clear();
        if (!text.isEmpty()) {
            blocks.append(buildBlock(QStringLiteral("paragraph"), text));
        }
    };

    auto flushList = [&]() {
        if (listLines.isEmpty()) {
            return;
        }
        const QString text = listLines.join(QStringLiteral("\n")).trimmed();
        listLines.clear();
        if (!text.isEmpty()) {
            blocks.append(buildBlock(QStringLiteral("list"), text));
        }
    };

    const QRegularExpression headingPattern(QStringLiteral(R"(^\s*(#{1,6})\s+(.+)$)"));

    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();

        if (inCodeBlock) {
            if (trimmed.startsWith(QStringLiteral("```"))) {
                blocks.append(buildBlock(QStringLiteral("code"),
                                         codeLines.join(QStringLiteral("\n")),
                                         codeLanguage));
                inCodeBlock = false;
                codeLanguage.clear();
                codeLines.clear();
            } else {
                codeLines.append(line);
            }
            continue;
        }

        if (trimmed.startsWith(QStringLiteral("```"))) {
            flushParagraph();
            flushList();
            inCodeBlock = true;
            codeLanguage = trimmed.mid(3).trimmed();
            continue;
        }

        if (trimmed.isEmpty()) {
            flushParagraph();
            flushList();
            continue;
        }

        const QRegularExpressionMatch headingMatch = headingPattern.match(line);
        if (headingMatch.hasMatch()) {
            flushParagraph();
            flushList();
            const QString marker = headingMatch.captured(1);
            const QString headingText = headingMatch.captured(2).trimmed();
            blocks.append(buildBlock(QStringLiteral("heading"),
                                     headingText,
                                     QString(),
                                     marker.length()));
            continue;
        }

        if (isListLine(line)) {
            flushParagraph();
            listLines.append(line.trimmed());
            continue;
        }

        flushList();
        paragraphLines.append(line);
    }

    if (inCodeBlock) {
        blocks.append(buildBlock(QStringLiteral("code"),
                                 codeLines.join(QStringLiteral("\n")),
                                 codeLanguage));
    }

    flushParagraph();
    flushList();

    if (blocks.isEmpty() && !rawText.trimmed().isEmpty()) {
        blocks.append(buildBlock(QStringLiteral("paragraph"), rawText.trimmed()));
    }

    return blocks;
}

QVariantMap MarkdownMessageParser::buildBlock(const QString& type,
                                              const QString& rawText,
                                              const QString& language,
                                              int level)
{
    QVariantMap block;
    block.insert(QStringLiteral("type"), type);
    block.insert(QStringLiteral("rawText"), rawText);
    block.insert(QStringLiteral("language"), language);
    block.insert(QStringLiteral("level"), level);
    if (type == QStringLiteral("code")) {
        block.insert(QStringLiteral("html"), buildCodeHtml(rawText, language));
    } else {
        block.insert(QStringLiteral("html"), QString());
    }
    return block;
}

bool MarkdownMessageParser::isListLine(const QString& line)
{
    static const QRegularExpression kListPattern(
        QStringLiteral(R"(^\s*(([-*+])|(\d+\.))\s+.+$)"));
    return kListPattern.match(line).hasMatch();
}

QString MarkdownMessageParser::buildCodeHtml(const QString& rawCode, const QString& language)
{
    QString text = rawCode;
    text.replace(QStringLiteral("\t"), QStringLiteral("    "));

    const QString lang = language.trimmed().toLower();
    QVector<QString> replacements;

    auto stash = [&](const QRegularExpression& pattern, const QString& color, bool bold = false) {
        text = replaceWithPattern(text, pattern, [&](const QString& captured) {
            const QString token = placeholderToken(replacements.size());
            replacements.append(wrapSpan(captured, color, bold));
            return token;
        });
    };

    stash(QRegularExpression(QStringLiteral(R"(/\*[\s\S]*?\*/)")), QStringLiteral("#6B7280"));
    stash(QRegularExpression(QStringLiteral(R"(//[^\n]*)")), QStringLiteral("#6B7280"));
    stash(QRegularExpression(QStringLiteral(R"("(?:\\.|[^"])*")")), QStringLiteral("#60A5FA"));
    stash(QRegularExpression(QStringLiteral(R"('(?:\\.|[^'])*')")), QStringLiteral("#60A5FA"));

    QStringList keywords = {
        QStringLiteral("if"), QStringLiteral("else"), QStringLiteral("for"), QStringLiteral("while"),
        QStringLiteral("switch"), QStringLiteral("case"), QStringLiteral("default"),
        QStringLiteral("break"), QStringLiteral("continue"), QStringLiteral("return"),
        QStringLiteral("class"), QStringLiteral("struct"), QStringLiteral("public"),
        QStringLiteral("private"), QStringLiteral("protected"), QStringLiteral("virtual"),
        QStringLiteral("override"), QStringLiteral("const"), QStringLiteral("static"),
        QStringLiteral("new"), QStringLiteral("delete"), QStringLiteral("try"),
        QStringLiteral("catch"), QStringLiteral("throw"), QStringLiteral("typename"),
        QStringLiteral("template"), QStringLiteral("using"), QStringLiteral("namespace"),
        QStringLiteral("this"), QStringLiteral("nullptr"), QStringLiteral("true"),
        QStringLiteral("false")
    };

    if (lang == QStringLiteral("python") || lang == QStringLiteral("py")) {
        keywords = QStringList{
            QStringLiteral("def"), QStringLiteral("class"), QStringLiteral("if"), QStringLiteral("elif"),
            QStringLiteral("else"), QStringLiteral("for"), QStringLiteral("while"), QStringLiteral("return"),
            QStringLiteral("try"), QStringLiteral("except"), QStringLiteral("finally"), QStringLiteral("with"),
            QStringLiteral("as"), QStringLiteral("import"), QStringLiteral("from"), QStringLiteral("pass"),
            QStringLiteral("break"), QStringLiteral("continue"), QStringLiteral("None"), QStringLiteral("True"),
            QStringLiteral("False"), QStringLiteral("lambda")
        };
    } else if (lang == QStringLiteral("javascript")
               || lang == QStringLiteral("js")
               || lang == QStringLiteral("typescript")
               || lang == QStringLiteral("ts")) {
        keywords = QStringList{
            QStringLiteral("function"), QStringLiteral("class"), QStringLiteral("if"), QStringLiteral("else"),
            QStringLiteral("for"), QStringLiteral("while"), QStringLiteral("switch"), QStringLiteral("case"),
            QStringLiteral("break"), QStringLiteral("continue"), QStringLiteral("return"), QStringLiteral("const"),
            QStringLiteral("let"), QStringLiteral("var"), QStringLiteral("new"), QStringLiteral("this"),
            QStringLiteral("try"), QStringLiteral("catch"), QStringLiteral("throw"), QStringLiteral("async"),
            QStringLiteral("await"), QStringLiteral("import"), QStringLiteral("export"), QStringLiteral("true"),
            QStringLiteral("false"), QStringLiteral("null"), QStringLiteral("undefined")
        };
    }

    const QRegularExpression keywordPattern(
        QStringLiteral("\\b(%1)\\b").arg(keywords.join(QStringLiteral("|"))));
    stash(keywordPattern, QStringLiteral("#E879F9"), true);
    stash(QRegularExpression(QStringLiteral(R"(\b(int|long|short|float|double|bool|char|void|QString|QVector|QList|QObject|size_t|std::string)\b)")),
          QStringLiteral("#F59E0B"));
    stash(QRegularExpression(QStringLiteral(R"(\b(\d+(?:\.\d+)?)\b)")),
          QStringLiteral("#34D399"));

    text = text.toHtmlEscaped();
    for (int i = 0; i < replacements.size(); ++i) {
        text.replace(placeholderToken(i), replacements.at(i));
    }

    return QStringLiteral("<pre style=\"margin:0;font-family:'Consolas';font-size:13px;line-height:1.45;color:rgb(226,232,240);\">%1</pre>")
        .arg(text);
}

QString MarkdownMessageParser::wrapSpan(const QString& text, const QString& color, bool bold)
{
    QString formatted = QStringLiteral("<font color=\"%1\">%2</font>")
                            .arg(color, text.toHtmlEscaped());
    if (bold) {
        formatted = QStringLiteral("<b>%1</b>").arg(formatted);
    }
    return formatted;
}

QString MarkdownMessageParser::replaceWithPattern(const QString& input,
                                                  const QRegularExpression& pattern,
                                                  const std::function<QString(const QString&)>& replacer)
{
    QString result;
    result.reserve(input.size() + 64);

    int cursor = 0;
    QRegularExpressionMatchIterator it = pattern.globalMatch(input);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        if (!match.hasMatch()) {
            continue;
        }

        const int start = match.capturedStart();
        const int end = match.capturedEnd();
        if (start < cursor) {
            continue;
        }

        result += input.mid(cursor, start - cursor);
        result += replacer(match.captured(0));
        cursor = end;
    }

    result += input.mid(cursor);
    return result;
}

QString MarkdownMessageParser::placeholderToken(int index)
{
    int value = index;
    QString suffix;
    do {
        suffix.prepend(QChar('A' + (value % 26)));
        value = value / 26 - 1;
    } while (value >= 0);

    return QStringLiteral("@@PH_%1@@").arg(suffix);
}
