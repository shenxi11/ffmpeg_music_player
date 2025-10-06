#include "music_list_widget_net.h"
#include "httprequest.h"
#include "play_widget.h"
#include <QSpacerItem>
MusicListWidgetNet::MusicListWidgetNet(QWidget *parent) : QWidget(parent)
{
    QWidget* top_widget = new QWidget(this);
    download_dir = new QPushButton(top_widget);
    download_dir->setText("下载路径:");
    download_dir->setFixedSize(100,30);
    download_dir->setStyleSheet("QPushButton { background-color: transparent; border: none; }");

    translateBtn = new QPushButton(top_widget);
    translateBtn->setText("语音转换");
    translateBtn->setFixedSize(100, 30);
    translateBtn->setStyleSheet(
                "QPushButton "
                "{ border-radius: 15px; border: 2px solid #667eea; "
                "  background-color: #667eea; color: white; }"
                );

    dir_label = new QLabel(top_widget);
    QHBoxLayout* top_layout = new QHBoxLayout(top_widget);
    top_layout->addWidget(download_dir);
    top_layout->addWidget(dir_label);
    top_layout->addStretch();
    top_layout->addWidget(translateBtn);

    top_widget->setLayout(top_layout);

    connect(download_dir, &QPushButton::clicked, this, &MusicListWidgetNet::signal_choose_download_dir);
    connect(translateBtn, &QPushButton::clicked, this, &MusicListWidgetNet::on_signal_translate_button_clicked);

    listWidget = new MusicListWidget(this);
    listWidget->resize(800, height() - 60);
    listWidget->clear();
    listWidget->show();

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

    QHBoxLayout* parameter_layout = new QHBoxLayout(this);
    parameter_layout->addWidget(first_widget);
    parameter_layout->addSpacerItem(spacer);
    parameter_layout->addWidget(duration_label);
    parameterWidget->setLayout(parameter_layout);

    QVBoxLayout* v_layout = new QVBoxLayout(this);
    v_layout->addWidget(top_widget);
    v_layout->addWidget(parameterWidget);
    v_layout->addWidget(listWidget);

    connect(this, &MusicListWidgetNet::signal_add_songlist, listWidget, &MusicListWidget::on_signal_add_songlist);
    connect(this, &MusicListWidgetNet::signal_add_songlist, [=](const QStringList songList, const QList<double> duration){
        for(int i = 0; i < songList.size(); i++)
        {
            song_duration[songList.at(i)] = duration.at(i);
        }
    });
    connect(listWidget, &MusicListWidget::play_click, this, &MusicListWidgetNet::on_signal_play_click);
    connect(listWidget, &MusicListWidget::remove_click, this, &MusicListWidgetNet::on_signal_remove_click);
    connect(listWidget, &MusicListWidget::signal_download_click, this, &MusicListWidgetNet::on_signal_download_music);


    connect(this, &MusicListWidgetNet::signal_play_button_click, listWidget, &MusicListWidget::receive_song_op);

    connect(this, &MusicListWidgetNet::signal_last, listWidget, &MusicListWidget::Last_click);
    connect(this, &MusicListWidgetNet::signal_next, listWidget, &MusicListWidget::Next_click);

    request = HttpRequestPool::getInstance().getRequest();
    connect(request, &HttpRequest::signal_streamurl, this,[=](bool flag, QString file){
        //qInfo()<<__FUNCTION__<<file;
        emit signal_play_click(file, true);
    });
}

void MusicListWidgetNet::on_signal_play_click(const QString name)
{
    double duration = song_duration[name];
    request->get_music_data(name);
}
void MusicListWidgetNet::on_signal_download_music(QString songName)
{
    request->Download(songName, down_dir);

}
void MusicListWidgetNet::on_signal_remove_click(const QString name)
{

}
void MusicListWidgetNet::on_signal_set_down_dir(QString down_dir)
{
    dir_label->setText(down_dir);
    this->down_dir = down_dir;
}
void MusicListWidgetNet::on_signal_play_button_click(bool flag, const QString filename)
{
    if(auto sender_ = dynamic_cast<PlayWidget*>(sender()))
    {
        if(sender_->get_net_flag())

            emit signal_play_button_click(flag, filename);
    }
}

void MusicListWidgetNet::on_signal_translate_button_clicked()
{
    emit signal_translate_button_clicked();
}
