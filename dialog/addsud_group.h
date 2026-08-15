#ifndef ADDSUD_GROUP_H
#define ADDSUD_GROUP_H

#include <QWidget>
#include <client.h>

namespace Ui {
class AddSud_Group;
}

class AddSud_Group : public QWidget
{
    Q_OBJECT

public:
    explicit AddSud_Group(QWidget *parent = nullptr);
    ~AddSud_Group();

private slots:
    void on_add_Btn_clicked();

    void on_group_comboBox_currentIndexChanged(int index);

    void on_suf_Btn_clicked();

private:
    void refreshgroup();

private:
    Ui::AddSud_Group *ui;
    Client &config;
};

#endif // ADDSUD_GROUP_H
