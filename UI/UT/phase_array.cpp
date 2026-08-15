#include "phase_array.h"

#include <app.h>

#include <QComboBox>
#include <QListView>

#include "ui_phase_array.h"

Phase_array::Phase_array(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Phase_array)
    , config(Client::getInstance())
{
    ui->setupUi(this);
    initWidget();
    initSlot();
}

Phase_array::~Phase_array()
{
    delete ui;
}

void Phase_array::initWidget()
{
    ui->ScanBox->setView(new QListView);
    ui->FocustypeBox->setView(new QListView);
    ui->MultiBox->setView(new QListView);
    ui->StepSpinBox->setDecimals(0);
    ui->ApertureSpinBox->setDecimals(0);
    ui->FirstSpinBox->setDecimals(0);
    ui->TransmissionSpinBox->setDecimals(0);
    ui->ReceptionSpinBox->setDecimals(0);
    // ui->TransmissionSpinBox->setReadOnly(true);
    // ui->ReceptionSpinBox->setReadOnly(true);
    //  QVariant v(0 | 32);
    //  QModelIndex index1 = ui->ScanBox->model()->index(1, 0);
    //  ui->ScanBox->model()->setData(index1, v, Qt::UserRole - 1);

    ui->ApertureSpinBox->installEventFilter(this);
    ui->StepSpinBox->installEventFilter(this);
    ui->FirstSpinBox->installEventFilter(this);
    ui->LastSpinBox->installEventFilter(this);
    ui->BeamSpinBox->installEventFilter(this);
    ui->FocusdepthSpinBox->installEventFilter(this);
    ui->TransmissionSpinBox->installEventFilter(this);
    ui->ReceptionSpinBox->installEventFilter(this);
}

void Phase_array::initSlot()
{
    connect(App::getInstance(), &App::refresh_Allpara, this, &Phase_array::getcurrentPara);
}

void Phase_array::getcurrentPara()
{
    ui->ScanBox->blockSignals(true);
    ui->MultiBox->blockSignals(true);
    ui->ApertureSpinBox->blockSignals(true);
    ui->StepSpinBox->blockSignals(true);
    ui->FirstSpinBox->blockSignals(true);
    ui->LastSpinBox->blockSignals(true);
    ui->BeamSpinBox->blockSignals(true);
    ui->FocustypeBox->blockSignals(true);
    ui->FocusdepthSpinBox->blockSignals(true);
    ui->TransmissionSpinBox->blockSignals(true);
    ui->ReceptionSpinBox->blockSignals(true);

    ui->MultiBox->setCurrentIndex(static_cast<int>(config.getBeamMode()));
    ui->ApertureSpinBox->setValue(config.getBeamAperture());

    if (config.getScanType() == scanType::LinearScan) {
        ui->ScanBox->setCurrentIndex(0);
        ui->Steplabel->setText(tr("Step"));
        ui->Firstlabel->setText(tr("First element"));
        ui->Lastlabel->setText(tr("Last element"));
        ui->StepSpinBox->setValue(config.getBeamElementStep());
        ui->BeamSpinBox->setValue(config.getBeamAngleLinear());
        ui->FirstSpinBox->setValue(config.getBeamFirstElement());
        ui->LastSpinBox->setValue(config.getBeamLastElement());
        ui->Beamlabel->show();
        ui->BeamSpinBox->show();
        ui->StepSpinBox->show();
        ui->FirstSpinBox->show();
        ui->LastSpinBox->show();
        ui->Steplabel->show();
        ui->Firstlabel->show();
        ui->Lastlabel->show();
        ui->ApertureSpinBox->show();
        ui->Aperturelabel->show();

    } else if (config.getScanType() == scanType::SectorialScan) {
        ui->ScanBox->setCurrentIndex(1);
        ui->Steplabel->setText(tr("Angle step"));
        ui->Firstlabel->setText(tr("Start angle"));
        ui->Lastlabel->setText(tr("End angle"));

        ui->StepSpinBox->setValue(config.getBeamAngleStep());
        ui->FirstSpinBox->setValue(config.getBeamAngleMin());
        ui->LastSpinBox->setValue(config.getBeamAngleMax());
        ui->Beamlabel->hide();
        ui->BeamSpinBox->hide();
        ui->StepSpinBox->show();
        ui->FirstSpinBox->show();
        ui->LastSpinBox->show();
        ui->Steplabel->show();
        ui->Firstlabel->show();
        ui->Lastlabel->show();
        ui->ApertureSpinBox->show();
        ui->Aperturelabel->show();
    } else if (config.getScanType() == scanType::UTMode) {
        ui->ScanBox->setCurrentIndex(2);
        ui->BeamSpinBox->setValue(config.getBeamAngleLinear());
        ui->ApertureSpinBox->hide();
        ui->Aperturelabel->hide();
        ui->StepSpinBox->hide();
        ui->FirstSpinBox->hide();
        ui->LastSpinBox->hide();
        ui->Steplabel->hide();
        ui->Firstlabel->hide();
        ui->Lastlabel->hide();
        ui->Beamlabel->show();
        ui->BeamSpinBox->show();
    }

    ui->FocusdepthSpinBox->setValue(config.getFocalPosition());
    ui->FocustypeBox->setCurrentIndex(static_cast<int>(config.getFocusMode()));
    ui->TransmissionSpinBox->setValue(config.getTransmissionChannel());
    ui->ReceptionSpinBox->setValue(config.getReceptionChannel());

    ui->ScanBox->blockSignals(false);
    ui->MultiBox->blockSignals(false);
    ui->StepSpinBox->blockSignals(false);
    ui->ApertureSpinBox->blockSignals(false);
    ui->FirstSpinBox->blockSignals(false);
    ui->LastSpinBox->blockSignals(false);
    ui->BeamSpinBox->blockSignals(false);
    ui->FocusdepthSpinBox->blockSignals(false);
    ui->FocustypeBox->blockSignals(false);
    ui->TransmissionSpinBox->blockSignals(false);
    ui->ReceptionSpinBox->blockSignals(false);
}

void Phase_array::changeEvent(QEvent *event)
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

void Phase_array::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
}

bool Phase_array::eventFilter(QObject *watched, QEvent *event)
{
    QList<QDoubleSpinBox *> doubleSpinBoxes = findChildren<QDoubleSpinBox *>();
    for (QDoubleSpinBox *spinBox : doubleSpinBoxes) {
        if (spinBox) {
            if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
                isBlockSignal[spinBox] = true;
            } else if (event->type() == QEvent::FocusOut
                       || event->type() == QEvent::MouseButtonPress) {
                isBlockSignal[spinBox] = false;
            } else if (event->type() == QEvent::Wheel) {
                isBlockSignal[spinBox] = true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void Phase_array::on_ScanBox_currentIndexChanged(int index)
{
    switch (index) {
    case 0: // 线性扫描
        config.setScanType(scanType::LinearScan);
        break;
    case 1: // 扇形扫描
        config.setScanType(scanType::SectorialScan);
        break;
    case 2: // UT模式
        config.setScanType(scanType::UTMode);
        break;
    default:
        break;
    }

    emit App::getInstance()->signal_probeChange();
    emit App::getInstance()->refresh_Allpara();
}

void Phase_array::on_MultiBox_currentIndexChanged(int index)
{
    config.setBeamMode(static_cast<BeamMode>(index));
}

void Phase_array::on_FocustypeBox_currentIndexChanged(int index)
{
    config.setFocusMode(static_cast<FocalType>(index));
}

void Phase_array::on_LastSpinBox_editingFinished()
{
    if (config.getScanType() == scanType::LinearScan) {
        config.setBeamLastElement(static_cast<int32_t>(ui->LastSpinBox->value()));
        emit App::getInstance()->signal_probeChange();
    } else {
        config.setBeamAngleMax(ui->LastSpinBox->value());
    }
    emit App::getInstance()->refresh_Allpara();
}

void Phase_array::on_FirstSpinBox_editingFinished()
{
    if (config.getScanType() == scanType::LinearScan) {
        config.setBeamFirstElement(static_cast<int32_t>(ui->FirstSpinBox->value()));
        emit App::getInstance()->signal_probeChange();
    } else {
        config.setBeamAngleMin(ui->FirstSpinBox->value());
    }
    emit App::getInstance()->refresh_Allpara();
}

void Phase_array::on_ApertureSpinBox_editingFinished()
{
    config.setBeamAperture(static_cast<int>(ui->ApertureSpinBox->value()));
    emit App::getInstance()->signal_probeChange();
    emit App::getInstance()->refresh_Allpara();
}

void Phase_array::on_BeamSpinBox_editingFinished()
{

    config.setBeamAngleLinear(ui->BeamSpinBox->value());
}

void Phase_array::on_FocusdepthSpinBox_editingFinished()
{
    config.setFocalPosition(static_cast<float>(ui->FocusdepthSpinBox->value()));
}

void Phase_array::on_TransmissionSpinBox_editingFinished()
{
    config.setTransmissionChannel(static_cast<int>(ui->TransmissionSpinBox->value()));
    emit App::getInstance()->refresh_Allpara();
}

void Phase_array::on_ReceptionSpinBox_editingFinished()
{
    config.setReceptionChannel(static_cast<int>(ui->ReceptionSpinBox->value()));
    emit App::getInstance()->refresh_Allpara();
}

void Phase_array::on_StepSpinBox_editingFinished()
{
    if (config.getScanType() == scanType::LinearScan) {
        config.setBeamElementStep(static_cast<int>(ui->StepSpinBox->value()));
        emit App::getInstance()->signal_probeChange();
    } else {
        config.setBeamAngleStep(ui->StepSpinBox->value());
    }
    emit App::getInstance()->refresh_Allpara();
}

void Phase_array::on_ApertureSpinBox_valueChanged(double aperture)
{
    if (!isBlockSignal[ui->ApertureSpinBox]) {
        config.setBeamAperture(static_cast<int>(aperture));
        emit App::getInstance()->signal_probeChange();
        emit App::getInstance()->refresh_Allpara();
    }
}

void Phase_array::on_StepSpinBox_valueChanged(double step)
{
    if (!isBlockSignal[ui->StepSpinBox]) {
        if (config.getScanType() == scanType::LinearScan) {
            config.setBeamElementStep(static_cast<int32_t>(step));
            emit App::getInstance()->signal_probeChange();
            emit App::getInstance()->refresh_Allpara();
        }
    }
}

void Phase_array::on_FirstSpinBox_valueChanged(double arg1)
{
    if (!isBlockSignal[ui->FirstSpinBox]) {
        config.setBeamFirstElement(static_cast<int32_t>(arg1));
        emit App::getInstance()->signal_probeChange();
        emit App::getInstance()->refresh_Allpara();
    }
}

void Phase_array::on_LastSpinBox_valueChanged(double arg1)
{
    if (!isBlockSignal[ui->LastSpinBox]) {
        config.setBeamLastElement(static_cast<int32_t>(arg1));
        emit App::getInstance()->signal_probeChange();
        emit App::getInstance()->refresh_Allpara();
    }
}

void Phase_array::on_FocusdepthSpinBox_valueChanged(double arg1)
{
    if (!isBlockSignal[ui->FocusdepthSpinBox]) {
        config.setFocalPosition(static_cast<float>(arg1));
    }
}
