# Qt Ground Station Client Project File
# For Qt 5.12.8 - Cross-platform Configuration

QT += core gui network widgets opengl

TARGET = ground_station_client
TEMPLATE = app

CONFIG += c++14
CONFIG += console
CONFIG -= app_bundle

INCLUDEPATH += $$PWD/include $$PWD/src \
    $$PWD/include/protocol $$PWD/include/network $$PWD/include/video $$PWD/include/ui

SOURCES += \
    src/main.cpp \
    src/logger.cpp \
    src/config.cpp \
    src/protocol/statemachine.cpp \
    src/network/streammanager.cpp \
    src/network/streamclient.cpp \
    src/network/tcphandler.cpp \
    src/network/udpreceiver.cpp \
    src/protocol/protocolparser.cpp \
    src/protocol/packetreassembly.cpp \
    src/video/videodecoder.cpp \
    src/video/videorenderer.cpp \
    src/video/framebuffer.cpp \
    src/video/videostats.cpp \
    src/ui/mainwindow.cpp \
    src/ui/cameralistwidget.cpp \
    src/video/videodisplaywidget.cpp \
    src/ui/connectionwidget.cpp \
    src/ui/connectionsettingsdialog.cpp \
    src/ui/camerparamswidget.cpp \
    src/ui/application.cpp \
    src/ui/loginwidget.cpp

HEADERS += \
    include/logger.h \
    include/config.h \
    include/protocol/statemachine.h \
    include/network/streammanager.h \
    include/network/streamclient.h \
    include/network/tcphandler.h \
    include/network/udpreceiver.h \
    include/protocol/protocolparser.h \
    include/protocol/packetreassembly.h \
    include/video/videodecoder.h \
    include/video/videorenderer.h \
    include/video/framebuffer.h \
    include/video/videostats.h \
    include/ui/mainwindow.h \
    include/ui/cameralistwidget.h \
    include/video/videodisplaywidget.h \
    include/ui/connectionwidget.h \
    include/ui/connectionsettingsdialog.h \
    include/ui/camerparamswidget.h \
    include/ui/application.h \
    include/ui/loginwidget.h

FORMS += \
    mainwindow.ui

win32 {
    DEFINES += _WIN32_WINNT=0x0601

    win32-g++ {
        # 优先使用 Qt 自带的 MinGW 运行时库，避免与 MSYS2 MinGW 冲突
        QMAKE_LFLAGS += -L"D:/software/Qt/Qt5.12.8/Tools/mingw730_64/x86_64-w64-mingw32/lib"
        LIBS += -lmingw32 -lmingwex -lmsvcrt -lkernel32 -luser32 -ladvapi32 -lshell32 -lmingwthrd
    }

    OPENCV_ROOT = $$(OPENCV_ROOT)
    isEmpty(OPENCV_ROOT) {
        mingw {
            # 使用 Qt MinGW 重新编译的 OpenCV
            exists(F:/Code/opencv/build-qt-mingw73/install) {
                OPENCV_ROOT = F:/Code/opencv/build-qt-mingw73/install
            } else: exists(C:/msys64/mingw64) {
                OPENCV_ROOT = C:/msys64/mingw64
            }
        }
        msvc {
            exists(F:/project/xiaoguopinggu/pseGIS_SDK_x64/opencv451) {
                OPENCV_ROOT = F:/project/xiaoguopinggu/pseGIS_SDK_x64/opencv451
            }
        }
    }

    !isEmpty(OPENCV_ROOT) {
        INCLUDEPATH += $$OPENCV_ROOT/include
        mingw {
            # 使用 Qt MinGW 编译的 OpenCV 路径
            exists($$OPENCV_ROOT/x64/mingw) {
                INCLUDEPATH += $$OPENCV_ROOT/include/opencv4
                LIBS += -L$$OPENCV_ROOT/x64/mingw/lib
                # 使用新编译的 OpenCV 4.14 库
                LIBS += -lopencv_core4140 -lopencv_imgcodecs4140 -lopencv_imgproc4140
            } else {
                INCLUDEPATH += $$OPENCV_ROOT/include/opencv4
                LIBS += -L$$OPENCV_ROOT/lib
                LIBS += -lopencv_core -lopencv_imgcodecs -lopencv_imgproc
            }
        }
        msvc {
            LIBS += -L$$OPENCV_ROOT/lib
            LIBS += -lopencv_world451
        }
        message(Using OpenCV from: $$OPENCV_ROOT)
    } else {
        message(WARNING: OPENCV_ROOT not set. Set OPENCV_ROOT environment variable to your OpenCV path.)
    }
}

unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += opencv4

    target.path = /usr/local/bin
    INSTALLS += target
}

message(Building ground_station_client...)
