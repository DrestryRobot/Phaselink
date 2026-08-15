#ifndef PHASE_ARRAY_H
#define PHASE_ARRAY_H

#include "qspinbox.h"
#include <QWidget>
#include <client.h>

namespace Ui {
class Phase_array;
}

class Phase_array : public QWidget
{
    Q_OBJECT

public:
    explicit Phase_array(QWidget *parent = nullptr);
    ~Phase_array();

private:
    void initWidget();
    void initSlot();
    void getcurrentPara();

protected:
    virtual void changeEvent(QEvent *event) Q_DECL_OVERRIDE;
    virtual void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
    bool eventFilter(QObject *watched, QEvent *event) override;
private slots:
    void on_ScanBox_currentIndexChanged(int index);
    void on_MultiBox_currentIndexChanged(int index);
    void on_FocustypeBox_currentIndexChanged(int index);
    void on_LastSpinBox_editingFinished();
    void on_FirstSpinBox_editingFinished();
    void on_ApertureSpinBox_editingFinished();
    void on_BeamSpinBox_editingFinished();
    void on_FocusdepthSpinBox_editingFinished();
    void on_TransmissionSpinBox_editingFinished();
    void on_ReceptionSpinBox_editingFinished();
    void on_StepSpinBox_editingFinished();

    void on_ApertureSpinBox_valueChanged(double aperture);
    void on_StepSpinBox_valueChanged(double step);
    void on_FirstSpinBox_valueChanged(double arg1);
    void on_LastSpinBox_valueChanged(double arg1);
    void on_FocusdepthSpinBox_valueChanged(double arg1);

private:
    Ui::Phase_array *ui;
    Client &config;
    QMap<QDoubleSpinBox *, bool> isBlockSignal;
};

#endif // PHASE_ARRAY_H
