#include "main_widget.h"

Main_Widget::Main_Widget(QWidget *parent) : QWidget(parent)
  ,w(nullptr)
    ,list(nullptr)
{
    resize(1000,600);

    w = new Play_Widget(this);
    w->setFixedSize(800,500);
    QRegion region(0, 350, w->width(), 350);
    w->setMask(region);
    w->move((this->width()-w->width())/2,this->height()-w->height());


    list = new MusicListWidget(this);
    list->setFixedSize(200,300);
    list->move(this->width()-200,0);
    list->clear();
    list->close();

    connect(list,&MusicListWidget::selectMusic,w,&Play_Widget::_play_list_music);

    connect(w,&Play_Widget::big_clicked,this,[=](bool checked){
        if(checked){
            for(int i = 350; i >= 0;i -= 10){
                QRegion region1(0, i, w->width(), i);
                w->setMask(region1);
//                QThread::msleep(50);
                update();
            }
        }else{
            QRegion region1(0, 350, w->width(), 350);
            w->setMask(region1);
        }
    });
}
void Main_Widget:: show_widget(){
    //w->show();
}
