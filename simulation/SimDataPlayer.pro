QT += core

CONFIG += console c++17
CONFIG -= app_bundle

# 构建目录层级较深时，强制 qmake 使用绝对路径（同 SoundScan.pro）
QMAKE_PROJECT_DEPTH = 0

TARGET = SimDataPlayer
TEMPLATE = app

SOURCES += \
    sim_main.cpp \
    SimDataPlayer.cpp

HEADERS += \
    SimDataPlayer.h
