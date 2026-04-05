#ifndef MARKDOWN_MESSAGE_PARSER_H
#define MARKDOWN_MESSAGE_PARSER_H

#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QRegularExpression>
#include <functional>

/**
 * @brief 将 Markdown 源文本解析为块级结构，供 UI 分块渲染。
 */
class MarkdownMessageParser
{
public:
    QVariantList parse(const QString& rawText) const;

private:
    static QVariantMap buildBlock(const QString& type,
                                  const QString& rawText,
                                  const QString& language = QString(),
                                  int level = 0);
    static bool isListLine(const QString& line);
    static QString buildCodeHtml(const QString& rawCode, const QString& language);
    static QString wrapSpan(const QString& text, const QString& color, bool bold = false);
    static QString replaceWithPattern(const QString& input,
                                      const QRegularExpression& pattern,
                                      const std::function<QString(const QString&)>& replacer);
    static QString placeholderToken(int index);
};

#endif // MARKDOWN_MESSAGE_PARSER_H
