QT += widgets

INCLUDEPATH += $${PWD}/include

HEADERS       = mainwindow.h \
                mdichild.h \
                path.h \
                qdia.h
SOURCES       = main.cpp \
                mainwindow.cpp \
                mdichild.cpp \
                path.cpp \
                qdia.cpp
RESOURCES     = undebug.qrc

LIBS += OleAut32.lib
