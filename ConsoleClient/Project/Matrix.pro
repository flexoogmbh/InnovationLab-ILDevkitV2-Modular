TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    Source/main.cpp \
    Source/tcpServer.cpp \
    Source/tcpclient.cpp \
    Source/usbclient.cpp \
    Source/application.cpp \

HEADERS += \
    Header/commands.hpp \
    Header/main.hpp \
    Header/tcpServer.hpp \
    Header/tcpclient.hpp \
    Header/usbclient.hpp \
    Header/application.hpp \
    Header/settings.hpp \

INCLUDEPATH += \
    Header \

DEFINES +=\

QMAKE_LFLAGS += -pthread
