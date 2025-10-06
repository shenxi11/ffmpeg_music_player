#include "lyrictextedit.h"
#include<QDebug>


void LyricTextEdit::highlightLine(int lineNumber)
{
    // 先清除之前高亮的行
    if (lastHighlightedLine >= 0) {
        QTextBlock block1 = this->document()->findBlockByNumber(lastHighlightedLine);
        if (block1.isValid()) {
            QTextCursor cursor(block1);
            cursor.select(QTextCursor::LineUnderCursor);
            resetLineFormat(cursor);
        }
    }

    // 高亮当前行
    QTextBlock block = this->document()->findBlockByNumber(lineNumber);
    if (block.isValid()) {
        QTextCursor cursor(block);
        cursor.select(QTextCursor::LineUnderCursor);
        highlightLineFormat(cursor);
        emit signal_current_lrc(cursor.selectedText());
        
        // 更新记录
        lastHighlightedLine = lineNumber;
    }
}

void LyricTextEdit::resetLineFormat(QTextCursor &cursor)
{
    cursor.select(QTextCursor::LineUnderCursor);
    QTextCharFormat format;
    format.setFontWeight(QFont::Normal);
    format.setFontPointSize(16);  // 恢复默认字号
    cursor.mergeCharFormat(format);
}

// 设置高亮行的格式
void LyricTextEdit::highlightLineFormat(QTextCursor &cursor)
{
    cursor.select(QTextCursor::LineUnderCursor);
    QTextCharFormat format;
    format.setFontWeight(QFont::Bold);
    format.setFontPointSize(20);  // 设置高亮字号
    cursor.mergeCharFormat(format);
}

void LyricTextEdit::disableScrollBar()
{
    QScrollBar *scrollBar = this->verticalScrollBar();
    scrollBar->setEnabled(false);
    scrollBar->setVisible(false);
}

int LyricTextEdit::getLineHeight(int fontSize)
{
    // 创建一个 QFont 对象，设置指定的字号
    QFont font;
    font.setPointSize(fontSize);

    // 使用 QFontMetrics 计算行高
    QFontMetrics metrics(font);
    return metrics.lineSpacing();  // 返回行高
}
int LyricTextEdit::getLineSpacing(int fontSize)
{
    QFont font;
    font.setPointSize(fontSize);
    QFontMetrics metrics(font);
    return metrics.lineSpacing();  // 行间距
}
void LyricTextEdit::scrollLines(int lines)
{
    if (lines == 0) return;
    
    // 计算目标行的位置
    QTextBlock targetBlock = this->document()->findBlockByNumber(lines);
    if (!targetBlock.isValid()) return;
    
    // 获取文本编辑器的视口高度
    int viewportHeight = this->viewport()->height();
    
    // 计算目标行应该在视口中的位置（居中偏上）
    int targetPosition = viewportHeight / 3;  // 在视口上三分之一处
    
    // 获取目标行的坐标
    QTextCursor cursor(targetBlock);
    QRect blockRect = this->cursorRect(cursor);
    
    // 计算需要滚动的距离
    int scrollDistance = blockRect.top() - targetPosition;
    
    // 执行滚动
    QScrollBar *scrollBar = this->verticalScrollBar();
    scrollBar->setValue(scrollBar->value() + scrollDistance);
}

LyricTextEdit::~LyricTextEdit()
{
    //qDebug()<<"Destruct LyricTextEdit";
}
