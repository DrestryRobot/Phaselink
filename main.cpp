#include <app.h>

#include <QApplication>
#include <QTranslator>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator lang;
    lang.load(":/phaselink_CN.qm");
    qApp->installTranslator(&lang);
    MainWindow w;
    w.setWindowTitle("Phaselink");

    w.show();

    App::getInstance()->setMainWindow(&w);
    return a.exec();
}
