QT       += core gui network charts widgets xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets axcontainer opengl openglwidgets

CONFIG += c++17

# 让 Release 构建也生成 PDB 调试符号（MSVC /Zi）。
# SDK(client.dll) 仅支持 Release CRT/Qt，Debug 构建会因跨 CRT 释放堆块在启动时崩溃，
# 因此断点调试请使用带符号的 Release 构建。
CONFIG += force_debug_info

# Force qmake to emit absolute paths in Makefiles (same as SoundScan.pro)
QMAKE_PROJECT_DEPTH = 0

TRANSLATIONS += $$PWD/phaselink_CN.ts $$PWD/phaselink_EN.ts

# Phaselink SDK include/lib paths (same as SoundScan.pro)
INCLUDEPATH += "C:\Phaselink\include"
LIBS += -L"C:\Phaselink\lib" -lclient

SOURCES += \
    UI/UT/acg_tcg_widget.cpp \
    UI/UT/essentialwidget.cpp \
    UI/UT/gate_widget.cpp \
    UI/UT/phase_array.cpp \
    UI/UT/ut_widget.cpp \
    UI/scanning.cpp \
    app.cpp \
    datadispatch.cpp \
    dialog/addsud_group.cpp \
    dialog/amplitudpalette.cpp \
    dialog/axis_utils.cpp \
    dialog/colordialog.cpp \
    dialog/dataprocessor.cpp \
    dialog/listwidget.cpp \
    dialog/measurewidget.cpp \
    dialog/packetdatasaver.cpp \
    dialog/parammanager.cpp \
    dialog/rulerwidget.cpp \
    dialog/sider.cpp \
    dialog/viewwidget.cpp \
    dialog/viewworker.cpp \
    main.cpp \
    mainwindow.cpp \
    ndtbase.cpp

HEADERS += \
    UI/UT/acg_tcg_widget.h \
    UI/UT/essentialwidget.h \
    UI/UT/gate_widget.h \
    UI/UT/phase_array.h \
    UI/UT/ut_widget.h \
    UI/scanning.h \
    app.h \
    datadispatch.h \
    dialog/adddeviceDialog.h \
    dialog/addsud_group.h \
    dialog/amplitudpalette.h \
    dialog/axis_utils.h \
    dialog/colordialog.h \
    dialog/dataprocessor.h \
    dialog/listwidget.h \
    dialog/measurewidget.h \
    dialog/packetdatasaver.h \
    dialog/parammanager.h \
    dialog/rulerwidget.h \
    dialog/sider.h \
    dialog/viewwidget.h \
    dialog/viewworker.h \
    mainwindow.h \
    ndtbase.h

FORMS += \
    UI/UT/acg_tcg_widget.ui \
    UI/UT/essentialwidget.ui \
    UI/UT/gate_widget.ui \
    UI/UT/phase_array.ui \
    UI/UT/ut_widget.ui \
    UI/scanning.ui \
    dialog/addsud_group.ui \
    dialog/amplitudpalette.ui \
    dialog/colordialog.ui \
    dialog/measurewidget.ui \
    dialog/rulerwidget.ui \
    dialog/sider.ui \
    dialog/viewwidget.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

CONFIG += console

win32 {
    CONFIG += console
    CONFIG -= app_bundle
}

RESOURCES += \
    Qss.qrc
