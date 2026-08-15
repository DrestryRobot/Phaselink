#ifndef SCANNING_H
#define SCANNING_H

#include <QWidget>
#include <ndtbase.h>
#include <app.h>
#include <QDebug>
#include <client.h>
#include <QListView>
#include "dialog/parammanager.h"
namespace Ui {
class Scanning;
}

class Scanning : public NdtBase
{
    Q_OBJECT

public:
    explicit Scanning(QWidget *parent = nullptr);
    ~Scanning();

private:
    void initWidget();
    void getCurrentPara();

private:
    virtual void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
    virtual void changeEvent(QEvent *event) Q_DECL_OVERRIDE;
private slots:
    void on_tabWidget_currentChanged(int index);
    void on_CscanData_Amp_currentIndexChanged(int index);
    void on_CscanData_TOF_currentIndexChanged(int index);

    void on_End_value_editingFinished();
    void on_SoundEnd_value_editingFinished();
    void on_Resolution_value_editingFinished();
    void on_SoundStart_value_editingFinished();
    void on_time_resolute_value_editingFinished();
    void on_time_lengt_value_editingFinished();
    void on_ScanStart_value_editingFinished();

    void on_comboBox_currentIndexChanged(int index);

    void on_btnX_Y_clicked();

    void on_btnY_X_clicked();

    void on_End_value_2_editingFinished();

    void on_Resolution_value_2_editingFinished();

    void on_tabWidget_2_currentChanged(int index);

    void on_Resolution_value_3_editingFinished();

    void on_End_value_3_editingFinished();

    void on_spinResolution_editingFinished();

    void on_spinResolution_index_editingFinished();

    void on_Resolution_X_editingFinished();

    void on_EncoderComboBox_currentIndexChanged(int index);

    void on_xDirCombox_currentIndexChanged(int index);

    void on_yDirCombox_currentIndexChanged(int index);

private:
    void loadParameters();     // 加载参数
    void saveParameters();     // 保存参数
    void connectSaveSignals(); // 连接保存信号
private:
    Ui::Scanning *ui;
    Client &config;
    QList<QWidget *> show_list;
    QList<QWidget *> hide_list;
    // 新增成员
    ParamManager *paramManager;
    bool isLoadingParams; // 防止保存时循环触发
};

#endif // SCANNING_H
