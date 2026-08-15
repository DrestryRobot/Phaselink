#include "addsud_group.h"

#include <app.h>

#include <QListView>

#include "ui_addsud_group.h"

AddSud_Group::AddSud_Group(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AddSud_Group)
    , config(Client::getInstance())
{
    ui->setupUi(this);
    ui->group_comboBox->setView(new QListView);
    connect(App::getInstance(), &App::signal_Connected, this, &AddSud_Group::refreshgroup);
}

AddSud_Group::~AddSud_Group()
{
    delete ui;
}

void AddSud_Group::on_add_Btn_clicked()
{
    if (config.copyGroup()) {
        refreshgroup();
    }
}

void AddSud_Group::on_group_comboBox_currentIndexChanged(int index)
{
    if (index < 0)
        return;
    QString itemData = ui->group_comboBox->itemText(index);
    int val = itemData.toInt();

    if (!config.setCurrentGroup(val))
        return;
    refreshgroup();
    emit App::getInstance()->signal_GateView_Refresh();
}

void AddSud_Group::on_suf_Btn_clicked()
{
    if (config.getGroupsNo().size() <= 1)
        return;
    QString itemData = ui->group_comboBox->currentText();
    int index = itemData.toInt();
    if (config.removeGroup(index)) {
        refreshgroup();
    }
}

void AddSud_Group::refreshgroup()
{
    ui->group_comboBox->blockSignals(true);
    ui->group_comboBox->clear();
    auto Groupsize = config.getGroupsNo();

    for (int i = 0; i < Groupsize.size(); i++) {
        ui->group_comboBox->addItem(QString::number(Groupsize.at(i)));
    }
    int id = config.getCurrentGroup();
    ui->group_comboBox->setCurrentText(QString::number(id));
    ui->group_comboBox->blockSignals(false);
    emit App::getInstance()->signal_GateView_Refresh();
    emit App::getInstance()->refresh_Allpara();
}
