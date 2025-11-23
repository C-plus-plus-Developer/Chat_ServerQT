QT += core gui widgets network

CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    Chat.cpp \
    DBmanager.cpp \
    server_main.cpp \
    ServerController.cpp

HEADERS += \
    mainwindow.h \
    Chat.h \
    DBmanager.h \
    User.h \
    Messages.h \
    sha1.h \
    ServerController.h

LIBS += -L/usr/lib/x86_64-linux-gnu
LIBS += -lmysqlcppconn
LIBS += -lmysqlclient
LIBS += -lssl -lcrypto -lpthread -ldl

QMAKE_CXXFLAGS += -D_GNU_SOURCE

QMAKE_CXXFLAGS += -std=c++17
QMAKE_CXXFLAGS += -O2
QMAKE_CXXFLAGS += -Wall
QMAKE_CXXFLAGS += -I/usr/include/mysql-cppconn-8
QMAKE_CXXFLAGS += -I/usr/include/mysql
QMAKE_CXXFLAGS += -I/usr/local/include

CONFIG(release, debug|release) {
    DESTDIR = release
    OBJECTS_DIR = release/.obj
    MOC_DIR = release/.moc
    RCC_DIR = release/.qrc
    UI_DIR = release/.ui
}

CONFIG(debug, debug|release) {
    DESTDIR = debug
    OBJECTS_DIR = debug/.obj
    MOC_DIR = debug/.moc
    RCC_DIR = debug/.qrc
    UI_DIR = debug/.ui
}
