QT       += core gui widgets network svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# 项目信息
TARGET = PVEClient
TEMPLATE = app
VERSION = 0.1.0

# 源文件
SOURCES += \
    src/main.cpp \
    src/core/ConnectionInfo.cpp \
    src/core/ConfigManager.cpp \
    src/core/ProcessRunner.cpp \
    src/core/ConnectionManager.cpp \
    src/pve/PveApiClient.cpp \
    src/pve/PveAuthManager.cpp \
    src/protocol/RdpLauncher.cpp \
    src/protocol/SpiceLauncher.cpp \
    src/ui/MainWindow.cpp \
    src/ui/LoginView.cpp \
    src/ui/WorkspaceView.cpp \
    src/ui/FlowLayout.cpp \
    src/ui/VmCard.cpp \
    src/ui/LoginDialog.cpp \
    src/ui/ConnectionListWidget.cpp \
    src/ui/ConnectionEditDialog.cpp \
    src/ui/SettingsDialog.cpp

HEADERS += \
    src/core/ConnectionInfo.h \
    src/core/ConfigManager.h \
    src/core/ProcessRunner.h \
    src/core/ConnectionManager.h \
    src/pve/PveApiClient.h \
    src/pve/PveAuthManager.h \
    src/protocol/AbstractLauncher.h \
    src/protocol/RdpLauncher.h \
    src/protocol/SpiceLauncher.h \
    src/ui/MainWindow.h \
    src/ui/LoginView.h \
    src/ui/WorkspaceView.h \
    src/ui/FlowLayout.h \
    src/ui/VmCard.h \
    src/ui/LoginDialog.h \
    src/ui/ConnectionListWidget.h \
    src/ui/ConnectionEditDialog.h \
    src/ui/SettingsDialog.h

RESOURCES += \
    resources/resources.qrc

# 头文件搜索路径
INCLUDEPATH += src

# 输出目录
DESTDIR = $$PWD/bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR = $$PWD/build/moc
RCC_DIR = $$PWD/build/rcc

# Windows Release 模式隐藏控制台
win32:CONFIG(release, debug|release): CONFIG -= console
