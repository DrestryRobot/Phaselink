#include "ut_widget.h"
#include "ui_ut_widget.h"

UT_widget::UT_widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::UT_widget)
{
    ui->setupUi(this);
}

UT_widget::~UT_widget()
{
    delete ui;
}

void UT_widget::on_tabWidget_currentChanged(int index)
{
    // if (index == 2) {
    //     emit App::getInstance()->signal_showhideGatestatus();
    // }
    // if (index == 3)
    //     ui->tcgwidget->setEnabled(false);
}
void UT_widget::changeEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
    QWidget::changeEvent(event);
}
