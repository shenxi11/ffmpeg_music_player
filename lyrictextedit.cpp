#include "lyrictextedit.h"
#include<QDebug>


void LyricTextEdit::highlightLine(int lineNumber) {

    QTextBlock block1 = this->document()->findBlockByNumber(this->currentLine);

    if (block1.isValid()) {
        // 创建一个光标并定位到该行
        QTextCursor cursor(block1);

        // 选中整行
        cursor.select(QTextCursor::LineUnderCursor);

        resetLineFormat(cursor);
    }

    // 获取指定行的 QTextBlock
    QTextBlock block = this->document()->findBlockByNumber(lineNumber);

    if (block.isValid()) {
        // 创建一个光标并定位到该行
        QTextCursor cursor(block);

        // 选中整行
        cursor.select(QTextCursor::LineUnderCursor);

        //qDebug()<<cursor.selectedText();

        highlightLineFormat(cursor);
    }


}

void LyricTextEdit::resetLineFormat(QTextCursor &cursor){
    cursor.select(QTextCursor::LineUnderCursor);
    QTextCharFormat format;
    format.setFontWeight(QFont::Normal);
    format.setFontPointSize(16);  // 恢复默认字号
    cursor.mergeCharFormat(format);
}

// 设置高亮行的格式
void LyricTextEdit::highlightLineFormat(QTextCursor &cursor) {
    cursor.select(QTextCursor::LineUnderCursor);
    QTextCharFormat format;
    format.setFontWeight(QFont::Bold);
    format.setFontPointSize(20);  // 设置高亮字号
    cursor.mergeCharFormat(format);
}

void LyricTextEdit::updateScrollBar() {
    // 获取文档的总高度
    int documentHeight = this->document()->size().height();

    // 获取视口的高度
    int viewportHeight = this->viewport()->height();

    // 设置滚动条的范围
    QScrollBar *scrollBar = this->verticalScrollBar();
    scrollBar->setRange(0, documentHeight - viewportHeight);

    // 如果文档的高度小于视口的高度，确保滚动条值为0
    if (documentHeight <= viewportHeight) {
        scrollBar->setValue(0);
    }
}
void LyricTextEdit::disableScrollBar() {
    QScrollBar *scrollBar = this->verticalScrollBar();
    scrollBar->setEnabled(false);
    scrollBar->setVisible(false);
}

int LyricTextEdit::getLineHeight(int fontSize) {
    // 创建一个 QFont 对象，设置指定的字号
    QFont font;
    font.setPointSize(fontSize);

    // 使用 QFontMetrics 计算行高
    QFontMetrics metrics(font);
    return metrics.lineSpacing();  // 返回行高
}
int LyricTextEdit::getLineSpacing(int fontSize)  {
    QFont font;
    font.setPointSize(fontSize);
    QFontMetrics metrics(font);
    return metrics.lineSpacing();  // 行间距
}
void LyricTextEdit::scrollLines(int lines) {
    int fontSize = 20;  // 假设你使用的是 16 号字体

    // 获取行间距（包括行高和额外的空间）
    int lineSpacing = getLineSpacing(fontSize);

    QScrollBar *scrollBar = this->verticalScrollBar();
    int currentValue = scrollBar->value();


    scrollBar->setValue(currentValue + lines*lineSpacing);


}

LyricTextEdit::~LyricTextEdit(){
    qDebug()<<"Destruct LyricTextEdit";
}
