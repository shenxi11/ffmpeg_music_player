#ifndef LYRICTEXTEDIT_H
#define LYRICTEXTEDIT_H

#include <QObject>
#include<QScrollBar>
#include<QTextBlock>
#include<QTextEdit>
#include <QTextCursor>
#include <QFontMetrics>

class LyricTextEdit : public QTextEdit {
    Q_OBJECT
public:
    LyricTextEdit(QWidget *parent = nullptr)
        : QTextEdit(parent), lastHighlightedLine(-1) { }

//    // 更新并高亮显示歌词的当前行
//    void updateHighlight() ;

    void scrollOneLine();

    void highlightLine(int lineNumber);

    void updateScrollBar();

    int currentLine=5;  // 当前的歌词行

    void disableScrollBar();
signals:

private:



    int lastHighlightedLine;  // 上一次高亮的行

    // 恢复行的默认格式
    void resetLineFormat(QTextCursor &cursor) ;
    // 设置高亮行的格式
    void highlightLineFormat(QTextCursor &cursor);

    int getLineHeight(int fontSize);

    int getLineSpacing(int fontSize);

};


#endif // LYRICTEXTEDIT_H
