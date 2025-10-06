#include "music_list_widget_local.h"
#include "play_widget.h"
#include <QSpacerItem>
MusicListWidgetLocal::MusicListWidgetLocal(QWidget *parent) : QWidget(parent)
{
    QWidget* parameterWidget = new QWidget(this);
    parameterWidget->resize(width(), 30);

    QWidget* first_widget = new QWidget(parameterWidget);
    QLabel* nameLabel = new QLabel("     标题", first_widget);
    QLabel* numberLabel = new QLabel("#", first_widget);
    QHBoxLayout* first_layout = new QHBoxLayout(this);
    first_layout->addWidget(numberLabel);
    first_layout->addWidget(nameLabel);
    first_widget->setLayout(first_layout);

    QSpacerItem *spacer = new QSpacerItem(300, parameterWidget->height(), QSizePolicy::Expanding, QSizePolicy::Minimum);

    //QWidget* second_widget = new QWidget(parameterWidget);
    QLabel* duration_label = new QLabel("时长", parameterWidget);
    QSpacerItem *spacer_1= new QSpacerItem(50, parameterWidget->height(), QSizePolicy::Expanding, QSizePolicy::Minimum);
    QLabel* size_label = new QLabel("大小", parameterWidget);

    QHBoxLayout* parameter_layout = new QHBoxLayout(this);
    parameter_layout->addWidget(first_widget);
    parameter_layout->addSpacerItem(spacer);
    parameter_layout->addWidget(duration_label);
    parameter_layout->addWidget(size_label);
    parameterWidget->setLayout(parameter_layout);

    add = new QPushButton(this);
    add->setFixedSize(100, 30);
    add->setText("添加");
    add->setStyleSheet(
                "QPushButton "
                "{ border-radius: 15px; border: 2px solid black; }"
                );

    translateBtn = new QPushButton(this);
    translateBtn->setFixedSize(100, 30);
    translateBtn->setText("语音转换");
    translateBtn->setStyleSheet(
                "QPushButton "
                "{ border-radius: 15px; border: 2px solid #667eea; "
                "  background-color: #667eea; color: white; }"
                );

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(add);
    buttonLayout->addWidget(translateBtn);
    buttonLayout->addStretch();

    QWidget* buttonWidget = new QWidget(this);
    buttonWidget->setLayout(buttonLayout);

    connect(add, &QPushButton::clicked, this, &MusicListWidgetLocal::on_signal_add_button_clicked);
    connect(translateBtn, &QPushButton::clicked, this, &MusicListWidgetLocal::on_signal_translate_button_clicked);

    listWidget = new MusicListWidget(this);
    listWidget->resize(width(), height() - 60);
    listWidget->clear();
    listWidget->show();

    QVBoxLayout* v_layout = new QVBoxLayout(this);
    v_layout->addWidget(buttonWidget);
    v_layout->addWidget(parameterWidget);
    v_layout->addWidget(listWidget);

    setLayout(v_layout);

    request = HttpRequestPool::getInstance().getRequest();

    connect(this, &MusicListWidgetLocal::signal_add_song,[=](QString filename, QString path){
        request->AddMusic(path);
        listWidget->on_signal_add_song(filename, path);
    });
    connect(this, &MusicListWidgetLocal::signal_play_button_click, listWidget, &MusicListWidget::receive_song_op);

    connect(listWidget, &MusicListWidget::play_click, this, &MusicListWidgetLocal::on_signal_play_click);
    connect(listWidget, &MusicListWidget::remove_click, this, &MusicListWidgetLocal::on_signal_remove_click);

    connect(this, &MusicListWidgetLocal::signal_last, listWidget, &MusicListWidget::Last_click);
    connect(this, &MusicListWidgetLocal::signal_next, listWidget, &MusicListWidget::Next_click);

    auto user = User::getInstance();
    connect(user, &User::signal_add_songs, this,[=](){
        qDebug()<<__FUNCTION__<<"login";
        listWidget->remove_all();
        for(auto i: user->get_music_path())
        {
            QFileInfo info = QFileInfo(i);
            listWidget->on_signal_add_song(info.fileName(), info.filePath());
        }
    });
}
void MusicListWidgetLocal::on_signal_add_button_clicked()
{
    emit signal_add_button_clicked();
}
void MusicListWidgetLocal::on_signal_add_song(const QString filename, const QString path)
{
    emit signal_add_song(filename, path);
}
void MusicListWidgetLocal::on_signal_play_button_click(bool flag, const QString filename)
{
    if(auto sender_ = dynamic_cast<PlayWidget*>(sender()))
    {
        if(!sender_->get_net_flag())
            emit signal_play_button_click(flag, filename);
    }
}
void MusicListWidgetLocal::on_signal_play_click(const QString songName)
{
    emit signal_play_click(songName, false);
}
void MusicListWidgetLocal::on_signal_remove_click(const QString songeName)
{
    emit signal_remove_click(songeName);
}

void MusicListWidgetLocal::on_signal_translate_button_clicked()
{
    emit signal_translate_button_clicked();
}
