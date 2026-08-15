#ifndef ESSENTIALWIDGET_H
#define ESSENTIALWIDGET_H

#include "qspinbox.h"
#include <QWidget>
#include <ndtbase.h>
#include <client.h>
#include <app.h>

namespace Ui {
class EssentialWidget;
}

class EssentialWidget : public NdtBase
{
    Q_OBJECT

public:
    explicit EssentialWidget(QWidget *parent = nullptr);

    ~EssentialWidget();
    void setshowmodel(NdtBase::ShowModel mode);

private:
    void initSlot();
    void initWidget();
    void getcurrentPara();

protected:
    virtual void changeEvent(QEvent *event) Q_DECL_OVERRIDE;
    virtual void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
    bool eventFilter(QObject *watched, QEvent *event) override;
private slots:
    void on_PolarityBox_currentIndexChanged(int index);
    void on_RectifierBox_currentIndexChanged(int index);
    void on_MamplitudeBox_currentIndexChanged(int index);
    void on_SampleBox_editingFinished();
    void on_FilterHighBox_currentIndexChanged(int index);
    void on_FilterLowBox_currentIndexChanged(int index);
    void on_GainSpinBox_editingFinished();
    void on_RangstartSpinBox_editingFinished();
    void on_RangendSpinBox_editingFinished();
    void on_wedgeXSpinBox_editingFinished();
    void on_DoffsetSpinBox_editingFinished();
    void on_PRFSpinBox_editingFinished();
    void on_VoltageSpinBox_editingFinished();
    void on_PwidthSpinBox_editingFinished();
    void on_MvelocitySpinBox_editingFinished();
    void on_DvelocitySpinBox_editingFinished();

    void on_Pitch_value_editingFinished();

    void on_ZeroPosition_editingFinished();

    void on_GainSpinBox_valueChanged(double gain);
    void on_RangstartSpinBox_valueChanged(double start);
    void on_RangendSpinBox_valueChanged(double end);
    void on_MvelocitySpinBox_valueChanged(double arg1);
    void on_DvelocitySpinBox_valueChanged(double arg1);
    void on_Pitch_value_valueChanged(double arg1);
    void on_wedgeXSpinBox_valueChanged(double arg1);
    void on_DoffsetSpinBox_valueChanged(double arg1);
    void on_PRFSpinBox_valueChanged(double prf);
    void on_VoltageSpinBox_valueChanged(double voltage);
    void on_PwidthSpinBox_valueChanged(double arg1);
    void on_ZeroPosition_valueChanged(double arg1);

    void on_FrequenceSpinBox_editingFinished();

    void on_FrequenceSpinBox_valueChanged(double arg1);
    void on_SampleBox_valueChanged(double arg1);
    void on_vedioFilterBox_currentIndexChanged(int index);
    void on_wedgeAngle_valueChanged(double arg1);
    void on_wedgeAngle_editingFinished();

private:
    Ui::EssentialWidget *ui;
    Client &config;
    QStringList m_FilterTypeList;
    QMap<QDoubleSpinBox *, bool> isBlockSignal;
};

#endif // ESSENTIALWIDGET_H
