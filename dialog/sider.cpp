#include "sider.h"
#include "ui_sider.h"

Sider::Sider(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Sider)
{
    ui->setupUi(this);
}

Sider::~Sider()
{
    delete ui;
}

QScrollBar *Sider::silder()
{
    return ui->horizontalScrollBar;
}

void Sider::on_horizontalScrollBar_valueChanged(int value)
{
    ui->label->setText(QString::number(value));
}
