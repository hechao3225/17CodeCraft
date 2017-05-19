TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    cdn.cpp \
    deploy.cpp \
    io.cpp \
    ga.cpp \
    solution.cpp

HEADERS += \
    deploy.h \
    graph.h \
    ga.h \
    zkw.h \
    solution.h \
    ac.h \
    isap.h

INCLUDEPATH += lib
