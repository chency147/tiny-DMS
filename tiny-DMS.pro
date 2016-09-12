TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    Dentry.cpp \
    function.cpp \
    Group.cpp \
    init.cpp \
    Inode.cpp \
    SuperBlock.cpp \
    User.cpp

DISTFILES += \
    disk.data \
    tiny-DMS.cbp \
    tiny-DMS.layout \
    tiny-DMS.pro.user \
    tiny-DMS.depend

HEADERS += \
    Dentry.h \
    Group.h \
    Inode.h \
    my_fs.h \
    SuperBlock.h \
    User.h
