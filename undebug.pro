QT += widgets

INCLUDEPATH += $${PWD}/include

HEADERS       = mainwindow.h \
                mdichild.h \
                path.h
SOURCES       = main.cpp \
                mainwindow.cpp \
                mdichild.cpp \
                path.cpp
RESOURCES     = undebug.qrc

LIBS += OleAut32.lib
