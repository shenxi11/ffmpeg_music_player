#include "main_widget.h"

Main_Widget::Main_Widget(QWidget *parent) : QWidget(parent)
  ,w(nullptr)
{
    w=new Play_Widget(this);
    w->setFixedSize(800,500);

    QRegion region(0, 350, w->width(), 350);
    w->setMask(region);

    resize(1000,600);

    w->move((this->width()-w->width())/2,this->height()-w->height());


    connect(w,&Play_Widget::big_clicked,this,[=](bool checked){
        if(checked){
            for(int i=350;i>=0;i-=10){
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
