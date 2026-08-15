#ifndef GATE_WIDGET_H
#define GATE_WIDGET_H

#include "ui_gate_widget.h"
#include <QWidget>
#include <QTimer>
#include <client.h>
#include <app.h>

namespace Ui {
class Gate_widget;
}
enum GATE { GATE_A, GATE_B, GATE_C, GATE_I };
enum GATE_Sync { Sync_false, Sync_gate_I, Sync_gate_A, Sync_gate_B };

class Gate_widget : public QWidget
{
    Q_OBJECT

public:
    explicit Gate_widget(QWidget *parent = nullptr);
    ~Gate_widget();

private:
    void initSlot();
    void initWidget();
    void getcurrentPara();
    void setBtnchecked(bool check);

protected:
    virtual void changeEvent(QEvent *event) Q_DECL_OVERRIDE;
    virtual void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
    bool eventFilter(QObject *watched, QEvent *event) override;
private slots:
    void on_Btn_right_clicked();
    void on_Btn_left_clicked();
    void on_SyncBox_currentIndexChanged(int index);
    void on_MeasureBox_currentIndexChanged(int index);
    void on_SynAcqisitBox_currentIndexChanged(int index);
    void on_StartSpinBox_editingFinished();
    void on_WidthSpinBox_editingFinished();
    void on_ThresholdSpinBox_editingFinished();

    void on_StartSpinBox_valueChanged(double start);
    void on_WidthSpinBox_valueChanged(double width);

    void on_ThresholdSpinBox_valueChanged(double threshold);

private:
    Ui::Gate_widget *ui;
    GATE currentGate = GATE::GATE_A; // 默认为闸门A；

    // GATE_Sync GateA_syrc = GATE_Sync::Sync_false;
    // GATE_Sync GateB_syrc = GATE_Sync::Sync_false;
    Client &config;
    QMap<QDoubleSpinBox *, bool> isBlockSignal;
};

#endif // GATE_WIDGET_H
