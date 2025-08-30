QT       += core gui multimedia network concurrent opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    controlbar.cpp \
    cplaywidget.cpp \
    desk_lrc_widget.cpp \
    downloadthread.cpp \
    glwidget.cpp \
    httprequest.cpp \
    loginwidget.cpp \
    lrc_analyze.cpp \
    lyrictextedit.cpp \
    main.cpp \
    main_widget.cpp \
    mini_controlbar.cpp \
    movie_decoder.cpp \
    music_item.cpp \
    music_list_widget.cpp \
    music_list_widget_local.cpp \
    music_list_widget_net.cpp \
    pianwidget.cpp \
    play_widget.cpp \
    process_slider.cpp \
    rotatingcircleimage.cpp \
    searchbox.cpp \
    setting_widget.cpp \
    take_pcm.cpp \
    translate_widget.cpp \
    worker.cpp

HEADERS += \
    controlbar.h \
    cplaywidget.h \
    desk_lrc_widget.h \
    downloadthread.h \
    glwidget.h \
    headers.h \
    httprequest.h \
    loginwidget.h \
    lrc_analyze.h \
    lyrictextedit.h \
    main_widget.h \
    mini_controlbar.h \
    movie_decoder.h \
    music.h \
    music_item.h \
    music_list_widget.h \
    music_list_widget_local.h \
    music_list_widget_net.h \
    pianwidget.h \
    play_widget.h \
    process_slider.h \
    rotatingcircleimage.h \
    searchbox.h \
    setting_widget.h \
    take_pcm.h \
    translate_widget.h \
    worker.h

FORMS +=


TRANSLATIONS += \
    untitled_zh_CN.ts

INCLUDEPATH += /opt/ffmpeg-4.4/include \
                /home/shen/WhissperProject/whisper.cpp/include
LIBS += -L/opt/ffmpeg-4.4/lib \
        -lavcodec \
        -lavformat \
        -lavutil \
        -lavdevice \
        -lswscale \
        -lswresample \
        -L/home/shen/WhissperProject/whisper.cpp/build/src -lwhisper


RESOURCES += \
    pic.qrc

DISTFILES +=
