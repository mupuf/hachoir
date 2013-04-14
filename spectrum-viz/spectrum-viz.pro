# Add more folders to ship with the application, here
folder_01.source = qml/spectrum-viz
folder_01.target = qml
DEPLOYMENTFOLDERS = folder_01

# Additional import path used to resolve QML modules in Creator's code model
QML_IMPORT_PATH =

# If your application uses the Qt Mobility libraries, uncomment the following
# lines and add the respective components to the MOBILITY variable.
# CONFIG += mobility
# MOBILITY +=

QMAKE_CXXFLAGS += "-std=c++0x -Wall -I../common/"

LIBS += -lboost_system
LIBS += -lboost_filesystem

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp \
    sensingnode.cpp \
    sensingserver.cpp \
    powerspectrum.cpp \
    ../common/radioeventtable.cpp \
    ../common/ret_entry.cpp

# Please do not modify the following two lines. Required for deployment.
include(qtquick2applicationviewer/qtquick2applicationviewer.pri)
qtcAddDeployment()

HEADERS += \
    sensingnode.h \
    sensingserver.h \
    powerspectrum.h \
    ../common/absoluteringbuffer.h \
    ../common/radioeventtable.h \
    ../common/ret_entry.h
