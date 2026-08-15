#include "app.h"
#include <QRect>
#include <QWidget>
#include <QDebug>

App::App(QObject *parent)
    : QObject { parent }
{
}

void App::setMainWindow(QWidget *w)
{
    this->mainWindow = w;
}

QRect App::getMainWindowRect()
{
    QRect rect(0, 0, 1920, 1080);
    if (this->mainWindow)
        rect = this->mainWindow->geometry();
    return rect;
}
