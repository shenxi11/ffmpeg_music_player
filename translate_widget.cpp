
#include "translate_widget.h"
#include <QPainter>
#include <QStyleOption>

TranslateWidget::TranslateWidget(QWidget *parent) : QWidget(parent)
{
	this->resize(500, 400);
	this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint);
	this->setAttribute(Qt::WA_TranslucentBackground);

	// 顶部标题栏和窗口控制按钮
	titleBarWidget = new QWidget(this);
	titleBarWidget->setFixedHeight(40);
	titleBarWidget->setStyleSheet("background: transparent;");

	titleLabel = new QLabel("音频转文字", titleBarWidget);
	titleLabel->setStyleSheet("color: #333; font-size: 20px; font-weight: bold;");
	titleLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);


	// 右上角三个按钮使用图片
	minimizeButton = new QPushButton(titleBarWidget);
	maximizeButton = new QPushButton(titleBarWidget);
	closeButton = new QPushButton(titleBarWidget);
	minimizeButton->setFixedSize(28, 28);
	maximizeButton->setFixedSize(28, 28);
	closeButton->setFixedSize(28, 28);
	minimizeButton->setToolTip("最小化");
	maximizeButton->setToolTip("最大化/还原");
	closeButton->setToolTip("关闭");
	minimizeButton->setText("");
	maximizeButton->setText("");
	closeButton->setText("");
	minimizeButton->setStyleSheet("QPushButton { border-image: url(:/new/prefix1/icon/方形未选中.png); background:transparent; border:none; } QPushButton:hover{background:#e0e0e0;}");
	maximizeButton->setStyleSheet("QPushButton { border-image: url(:/new/prefix1/icon/减号.png); background:transparent; border:none; } QPushButton:hover{background:#e0e0e0;}");
	closeButton->setStyleSheet("QPushButton { border-image: url(:/new/prefix1/icon/关闭1.png); background:transparent; border:none; } QPushButton:hover{background:#ffeaea;}");



	titleBarLayout = new QHBoxLayout(titleBarWidget);
	titleBarLayout->addWidget(titleLabel);
	titleBarLayout->addStretch();
	titleBarLayout->addWidget(minimizeButton);
	titleBarLayout->addWidget(maximizeButton);
	titleBarLayout->addWidget(closeButton);
	titleBarLayout->setContentsMargins(0, 0, 0, 0);
	titleBarLayout->setSpacing(2);


	// 文件选择
	fileLabel = new QLabel("音频文件:", this);
	filePathEdit = new QLineEdit(this);
	filePathEdit->setPlaceholderText("请选择音频文件...");
	browseButton = new QPushButton("浏览", this);
	browseButton->setStyleSheet("background-color: #1DB954; color: white; border-radius: 4px; padding: 4px 12px;");
	fileLayout = new QHBoxLayout();
	fileLayout->addWidget(fileLabel);
	fileLayout->addWidget(filePathEdit);
	fileLayout->addWidget(browseButton);

	// 模型选择
	modelLabel = new QLabel("模型:", this);
	modelCombo = new QComboBox(this);
	modelCombo->addItems({"tiny", "base", "small", "medium", "large"});
	modelLayout = new QHBoxLayout();
	modelLayout->addWidget(modelLabel);
	modelLayout->addWidget(modelCombo);

	// 输出格式选择
	formatLabel = new QLabel("输出格式:", this);
	formatCombo = new QComboBox(this);
	formatCombo->addItems({"txt", "srt", "vtt", "json"});
	formatLayout = new QHBoxLayout();
	formatLayout->addWidget(formatLabel);
	formatLayout->addWidget(formatCombo);

	// 音频语言选择
	langLabel = new QLabel("音频语言:", this);
	langCombo = new QComboBox(this);
	langCombo->addItems({"自动检测", "中文", "英文", "日语", "韩语", "法语", "德语", "西班牙语"});
	langLayout = new QHBoxLayout();
	langLayout->addWidget(langLabel);
	langLayout->addWidget(langCombo);

	// 转换按钮
	transcribeButton = new QPushButton("开始转换", this);
	transcribeButton->setStyleSheet("background-color: #FF4766; color: white; border-radius: 4px; font-size: 16px; padding: 6px 0;");

	// 进度条
	progressBar = new QProgressBar(this);
	progressBar->setRange(0, 100);
	progressBar->setValue(0);
	progressBar->setTextVisible(true);
	progressBar->setStyleSheet("QProgressBar{border-radius:6px; background:#f0f0f0; border:1px solid #e0e0e0;} QProgressBar::chunk{background-color:#1DB954; border-radius:6px;}");

	// 结果显示
	resultEdit = new QTextEdit(this);
	resultEdit->setPlaceholderText("转换结果将在此显示...");
	resultEdit->setReadOnly(true);
	resultEdit->setStyleSheet("background: #f8f8f8; border-radius: 6px; border: 1px solid #e0e0e0;");

	// 主布局
	mainLayout = new QVBoxLayout(this);
	mainLayout->addWidget(titleBarWidget);
	mainLayout->addLayout(fileLayout);
	mainLayout->addLayout(modelLayout);
	mainLayout->addLayout(formatLayout);
	mainLayout->addLayout(langLayout);
	mainLayout->addWidget(transcribeButton);
	mainLayout->addWidget(progressBar);
	mainLayout->addWidget(resultEdit);
	mainLayout->setSpacing(14);
	mainLayout->setContentsMargins(18, 18, 18, 18);
	setLayout(mainLayout);

	connect(browseButton, &QPushButton::clicked, this, [=]() {
		QString fileName = QFileDialog::getOpenFileName(this, "选择音频文件", "", "音频文件 (*.wav *.mp3 *.aac *.flac)");
		if (!fileName.isEmpty()) {
			filePathEdit->setText(fileName);
		}
	});

	connect(maximizeButton, &QPushButton::clicked, this, [=]() {
		this->showMinimized();
	});
	connect(minimizeButton, &QPushButton::clicked, this, [=]() {
		if (isMaximized()) {
			showNormal();
		} else {
			showMaximized();
		}
	});
	connect(closeButton, &QPushButton::clicked, this, [=]() {
		this->close();
	});
}
// 鼠标拖动窗口实现
void TranslateWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && event->pos().y() <= 40) {
        mousePressed = true;
        mouseStartPoint = event->globalPos();
        windowStartPoint = this->frameGeometry().topLeft();
    }
    QWidget::mousePressEvent(event);
}

void TranslateWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (mousePressed) {
        QPoint distance = event->globalPos() - mouseStartPoint;
        this->move(windowStartPoint + distance);
    }
    QWidget::mouseMoveEvent(event);
}

void TranslateWidget::mouseReleaseEvent(QMouseEvent *event)
{
    mousePressed = false;
    QWidget::mouseReleaseEvent(event);
}
void TranslateWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// 主体背景：明亮白色，带柔和阴影
	QRect bgRect = rect().adjusted(2, 2, -2, -2);
	QColor bgColor(255, 255, 255, 250);
	painter.setBrush(bgColor);
	painter.setPen(Qt::NoPen);
	painter.drawRoundedRect(bgRect, 16, 16);

	// 顶部渐变条：淡粉到淡绿，柔和且不突兀
	QRect gradRect = QRect(bgRect.left(), bgRect.top(), bgRect.width(), 48);
	QLinearGradient grad(gradRect.topLeft(), gradRect.bottomLeft());
	grad.setColorAt(0, QColor(255, 71, 102, 80));   // 淡粉
	grad.setColorAt(1, QColor(29, 185, 84, 40));    // 淡绿
	painter.setBrush(grad);
	painter.setPen(Qt::NoPen);
	painter.drawRoundedRect(gradRect, 16, 16);

	// 四角圆角遮罩，避免渐变溢出
	QRegion maskRegion(bgRect, QRegion::Ellipse);
	painter.setClipRegion(QRegion(bgRect, QRegion::Rectangle));

	// 保持子控件样式
	QStyleOption opt;
	opt.init(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);
}
