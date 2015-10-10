INCLUDEPATH += $$PWD
SOURCES += $$PWD/openglwindow.cpp \
    gamewindow.cpp \
    camera.cpp \
    mytcpserver.cpp
HEADERS += $$PWD/openglwindow.h \
    gamewindow.h \
    camera.h \
    mytcpserver.h

SOURCES += \
    main.cpp

target.path = .
INSTALLS += target

RESOURCES += \
    gestionnaire.qrc

QT += network
