#include "main_widget.h"

Main_Widget::Main_Widget(QWidget *parent) : QWidget(parent)
  ,w(nullptr)
  ,list(nullptr)
{
    resize(1000,600);

    add = new QPushButton(this);
    add->setFixedSize(100, 50);
    add->move((this->width()-800)/2, 50);
    add->setText("添加");
    add->setStyleSheet(
        "QPushButton "
        "{ border-radius: 15px; border: 2px solid black; }"
        );

    main_list = new MusicListWidget(this);
    main_list->setFixedSize(800,350);
    main_list->move((this->width()-800)/2,this->height()- 500);
    main_list->clear();
    main_list->show();


    list = new MusicListWidget(this);
    list->setFixedSize(200,300);
    list->move(this->width()-200,0);
    list->clear();
    list->close();


    w = new Play_Widget(this);
    w->setFixedSize(800,500);
    QRegion region(0, 350, w->width(), 350);
    w->setMask(region);
    w->move((this->width()-w->width())/2,this->height()-w->height());

    connect(add, &QPushButton::clicked, w, &Play_Widget::openfile);

    connect(w,&Play_Widget::list_show,[=](bool flag){
        if(flag)
        {
            list->show();
        }
        else
        {
            list->close();
        }
    });

    connect(w, &Play_Widget::Next, list, &MusicListWidget::Next_click);
    connect(w, &Play_Widget::Last, list, &MusicListWidget::Last_click);

    connect(w,&Play_Widget::add_song,list,&MusicListWidget::addSong);
    connect(w,&Play_Widget::add_song,main_list,&MusicListWidget::addSong);
    connect(w, &Play_Widget::play_button_click,list,&MusicListWidget::receive_song_op);
    connect(w, &Play_Widget::play_button_click,main_list,&MusicListWidget::receive_song_op);

//    connect(list,&MusicListWidget::selectMusic,w,&Play_Widget::_play_list_music);
//    connect(main_list,&MusicListWidget::selectMusic,w,&Play_Widget::_play_list_music);

    connect(list, &MusicListWidget::play_click, w, &Play_Widget::_play_click);
    connect(main_list, &MusicListWidget::play_click, w, &Play_Widget::_play_click);

    connect(list, &MusicListWidget::remove_click, w, &Play_Widget::_remove_click);
    connect(main_list, &MusicListWidget::remove_click, w, &Play_Widget::_remove_click);

    connect(w,&Play_Widget::big_clicked,this,[=](bool checked){
        if(checked)
        {
            w->raise();
            for(int i = 350; i >= 0;i -= 10)
            {
                QRegion region1(0, i, w->width(), i);
                w->setMask(region1);

                update();
            }

        }
        else
        {
            w->lower();
            QRegion region1(0, 350, w->width(), 350);
            w->setMask(region1);

        }
    });
}

void Main_Widget::Update_paint()
{
    main_list->update();
    list->update();
}

