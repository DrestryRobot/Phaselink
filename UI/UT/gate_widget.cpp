#include "gate_widget.h"

#include <app.h>

#include <QDebug>
#include <QListView>

#include "ui_gate_widget.h"

Gate_widget::Gate_widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Gate_widget)
    , config(Client::getInstance())
{
    ui->setupUi(this);
    initWidget();
    initSlot();
}

Gate_widget::~Gate_widget()
{
    delete ui;
}

void Gate_widget::initSlot()
{
    connect(ui->GateBtnGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this,
            [=](QAbstractButton *Btn) {
                auto num = ui->GateBtnGroup->id(Btn);
                if (num == 0) {
                    currentGate = GATE::GATE_A;
                } else if (num == 1) {
                    currentGate = GATE::GATE_B;
                } else if (num == 2) {
                    currentGate = GATE::GATE_C;
                } else if (num == 3) {
                    currentGate = GATE::GATE_I;
                }
                getcurrentPara();
            });
    connect(App::getInstance(), &App::refresh_Allpara, this, &Gate_widget::getcurrentPara);

    connect(App::getInstance(), &App::signal_Connected, this, [this] {
        bool sync = config.getGateISyncSample();
        if (sync == false) {
            auto range_start = config.getRangeStart();
            if (range_start > config.getGateIStart()) {
                config.setRangeStart(range_start - config.getGateIStart());
            }
        }
    });
}

void Gate_widget::initWidget()
{
    for (int i = 0; i < ui->GateBtnGroup->buttons().size(); i++) {
        ui->GateBtnGroup->setId(ui->GateBtnGroup->buttons().at(i), i);
        if (ui->GateBtnGroup->buttons().at(i)->isChecked()) {
            currentGate = static_cast<GATE>(i);
        }
    }

    ui->SyncBox->setView(new QListView);
    ui->MeasureBox->setView(new QListView);
    ui->SynAcqisitBox->setView(new QListView);

    ui->StartSpinBox->installEventFilter(this);
    ui->WidthSpinBox->installEventFilter(this);
    ui->ThresholdSpinBox->installEventFilter(this);
}

void Gate_widget::getcurrentPara()
{
    ui->Btn_widget->blockSignals(true);
    ui->SyncBox->blockSignals(true);
    ui->StartSpinBox->blockSignals(true);
    ui->WidthSpinBox->blockSignals(true);
    ui->ThresholdSpinBox->blockSignals(true);
    ui->MeasureBox->blockSignals(true);
    ui->SynAcqisitBox->blockSignals(true);
    ui->Btn_left->blockSignals(true);
    ui->Btn_right->blockSignals(true);
    QListView *view = qobject_cast<QListView *>(ui->SyncBox->view());

    if (currentGate == GATE::GATE_I) {
        view->setRowHidden(3, true);
        view->setRowHidden(2, true);
        view->setRowHidden(1, true);

        if (config.getGateIEnable()) {
            ui->Btn_left->setChecked(true);
        } else {
            ui->Btn_right->setChecked(true);
        }
        ui->StartSpinBox->setValue(config.getGateIStart());
        ui->WidthSpinBox->setValue(config.getGateIEnd() - config.getGateIStart());
        auto threshold_I = config.getGateIThreshold();
        ui->ThresholdSpinBox->setValue(threshold_I);
        ui->MeasureBox->setCurrentIndex(static_cast<int>(config.getGateIMeasureType()));
        ui->SynAcqisitBox->show();
        ui->SynAcqisitlabel->show();

        ui->SynAcqisitBox->setCurrentIndex(config.getGateISyncSample());
        ui->SyncBox->setCurrentIndex(0);

    } else if (currentGate == GATE::GATE_A) {
        view->setRowHidden(3, true);
        view->setRowHidden(2, true);
        view->setRowHidden(1, false);

        // 同步模式加载：如果同步目标未使能，回退到Pulser
        auto syncA = config.getGateASynchronMode();
        if (syncA == GateSynchron::GateI && !config.getGateIEnable()) {
            syncA = GateSynchron::Pulser;
            config.setGateASynchronMode(GateSynchron::Pulser);
        }
        // 闸门A的SyncBox只有Pulser(0)和GateI(1)，确保index合法
        ui->SyncBox->setCurrentIndex(static_cast<int>(syncA) <= 1 ? static_cast<int>(syncA) : 0);
        ui->StartSpinBox->setValue(config.getGateAStart());

        if (config.getGateAEnable()) {
            ui->Btn_left->setChecked(true);
        } else {
            ui->Btn_right->setChecked(true);
        }

        ui->WidthSpinBox->setValue(config.getGateAEnd() - config.getGateAStart());
        auto threshold_A = config.getGateAThreshold();
        ui->ThresholdSpinBox->setValue(threshold_A);
        ui->MeasureBox->setCurrentIndex(static_cast<int>(config.getGateAMeasureType()));

        ui->SynAcqisitBox->hide();
        ui->SynAcqisitlabel->hide();

    } else if (currentGate == GATE::GATE_B) {
        view->setRowHidden(2, false);
        view->setRowHidden(1, false);
        view->setRowHidden(3, true);

        // 同步模式加载：如果同步目标未使能，回退到Pulser
        auto syncB = config.getGateBSynchronMode();
        if (syncB == GateSynchron::GateI && !config.getGateIEnable()) {
            syncB = GateSynchron::Pulser;
            config.setGateBSynchronMode(GateSynchron::Pulser);
        } else if (syncB == GateSynchron::GateA && !config.getGateAEnable()) {
            syncB = GateSynchron::Pulser;
            config.setGateBSynchronMode(GateSynchron::Pulser);
        }
        ui->SyncBox->setCurrentIndex(static_cast<int>(syncB));

        ui->StartSpinBox->setValue(config.getGateBStart());
        if (config.getGateBEnable()) {
            ui->Btn_left->setChecked(true);
        } else {
            ui->Btn_right->setChecked(true);
        }

        ui->WidthSpinBox->setValue(config.getGateBEnd() - config.getGateBStart());
        auto threshold_B = config.getGateBThreshold();
        ui->ThresholdSpinBox->setValue(threshold_B);
        ui->MeasureBox->setCurrentIndex(static_cast<int>(config.getGateBMeasureType()));
        ui->SynAcqisitBox->hide();
        ui->SynAcqisitlabel->hide();

    } else if (currentGate == GATE::GATE_C) {
        view->setRowHidden(2, false);
        view->setRowHidden(1, false);
        view->setRowHidden(3, false);

        auto syncC = config.getGateCSynchronMode();
        if (syncC == GateSynchron::GateI && !config.getGateIEnable()) {
            syncC = GateSynchron::Pulser;
            config.setGateCSynchronMode(GateSynchron::Pulser);
        } else if (syncC == GateSynchron::GateA && !config.getGateAEnable()) {
            syncC = GateSynchron::Pulser;
            config.setGateCSynchronMode(GateSynchron::Pulser);
        } else if (syncC == GateSynchron::GateB && !config.getGateBEnable()) {
            syncC = GateSynchron::Pulser;
            config.setGateCSynchronMode(GateSynchron::Pulser);
        }
        ui->SyncBox->setCurrentIndex(static_cast<int>(syncC));

        ui->StartSpinBox->setValue(config.getGateCStart());
        if (config.getGateCEnable()) {
            ui->Btn_left->setChecked(true);
        } else {
            ui->Btn_right->setChecked(true);
        }

        ui->WidthSpinBox->setValue(config.getGateCEnd() - config.getGateCStart());
        auto threshold_C = config.getGateCThreshold();
        ui->ThresholdSpinBox->setValue(threshold_C);
        ui->MeasureBox->setCurrentIndex(static_cast<int>(config.getGateCMeasureType()));
        ui->SynAcqisitBox->hide();
        ui->SynAcqisitlabel->hide();
    }

    ui->Btn_widget->blockSignals(false);
    ui->SyncBox->blockSignals(false);
    ui->StartSpinBox->blockSignals(false);
    ui->WidthSpinBox->blockSignals(false);
    ui->ThresholdSpinBox->blockSignals(false);
    ui->MeasureBox->blockSignals(false);
    ui->SynAcqisitBox->blockSignals(false);
    ui->Btn_left->blockSignals(false);
    ui->Btn_right->blockSignals(false);
}

void Gate_widget::setBtnchecked(bool check)
{
    if (check) {
        ui->Btn_left->click();
    } else {
        ui->Btn_right->click();
    }
}

void Gate_widget::changeEvent(QEvent *event)
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

void Gate_widget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
}

bool Gate_widget::eventFilter(QObject *watched, QEvent *event)
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

void Gate_widget::on_Btn_right_clicked()
{
    if (currentGate == GATE::GATE_I) {
        config.setGateIEnable(false);
        if (config.getGateASynchronMode() == GateSynchron::GateI) {
            config.setGateASynchronMode(GateSynchron::Pulser);
        }
        if (config.getGateBSynchronMode() == GateSynchron::GateI) {
            config.setGateBSynchronMode(GateSynchron::Pulser);
        }
        if (config.getGateCSynchronMode() == GateSynchron::GateI) {
            config.setGateCSynchronMode(GateSynchron::Pulser);
        }
        if (config.getGateASynchronMode() != GateSynchron::GateI
            && config.getGateBSynchronMode() != GateSynchron::GateI
            && config.getGateCSynchronMode() != GateSynchron::GateI) {
            config.setGateIMeasureType(measureType::MaxPeak);
        }
    } else if (currentGate == GATE::GATE_A) {
        config.setGateAEnable(false);
        if (config.getGateBSynchronMode() == GateSynchron::GateA) {
            config.setGateBSynchronMode(GateSynchron::Pulser);
        }
        if (config.getGateCSynchronMode() == GateSynchron::GateA) {
            config.setGateCSynchronMode(GateSynchron::Pulser);
        }
    } else if (currentGate == GATE::GATE_B) {
        config.setGateBEnable(false);
        if (config.getGateCSynchronMode() == GateSynchron::GateB) {
            config.setGateCSynchronMode(GateSynchron::Pulser);
        }
    } else if (currentGate == GATE::GATE_C) {
        config.setGateCEnable(false);
    }
    emit App::getInstance()->signal_showhideGatestatus();
}

void Gate_widget::on_Btn_left_clicked()
{
    if (currentGate == GATE::GATE_I) {
        config.setGateIEnable(true);
    } else if (currentGate == GATE::GATE_A) {
        config.setGateAEnable(true);
    } else if (currentGate == GATE::GATE_B) {
        config.setGateBEnable(true);
    } else if (currentGate == GATE::GATE_C) {
        config.setGateCEnable(true);
    }
    emit App::getInstance()->signal_showhideGatestatus();
}

void Gate_widget::on_SyncBox_currentIndexChanged(int index)
{
    if (currentGate == GATE::GATE_I) {
    } else if (currentGate == GATE::GATE_A) {
        // index: 0=Pulser, 1=GateI
        if (index == 1 && config.getGateIEnable()) {
            // 闸门I使能时，闸门A可同步GateI
            config.setGateASynchronMode(GateSynchron::GateI);
        } else {
            // 闸门I未使能时，回退到Pulser
            config.setGateASynchronMode(GateSynchron::Pulser);
            if (index == 1) {
                // UI修正：闸门I未使能，回退SyncBox显示
                ui->SyncBox->blockSignals(true);
                ui->SyncBox->setCurrentIndex(0);
                ui->SyncBox->blockSignals(false);
            }
        }
        auto newStart = ui->StartSpinBox->value();
        auto newEnd = ui->WidthSpinBox->value() + newStart;
        bool flag = config.setGateAEnd(newEnd);
        if (flag == false) {
            config.setGateAStart(newStart);
            config.setGateAEnd(newEnd);
        }
        config.setGateAStart(newStart);

    } else if (currentGate == GATE::GATE_B) {
        // index: 0=Pulser, 1=GateI, 2=GateA
        if (index == 1 && config.getGateIEnable()) {
            // 闸门I使能时，闸门B可同步GateI
            config.setGateBSynchronMode(GateSynchron::GateI);
        } else if (index == 2 && config.getGateAEnable()) {
            // 闸门A使能时，闸门B可同步GateA
            config.setGateBSynchronMode(GateSynchron::GateA);
        } else {
            // 闸门I/A未使能时，回退到Pulser
            config.setGateBSynchronMode(GateSynchron::Pulser);
            if ((index == 1 && !config.getGateIEnable())
                || (index == 2 && !config.getGateAEnable())) {
                // UI修正：同步目标未使能，回退SyncBox显示
                ui->SyncBox->blockSignals(true);
                ui->SyncBox->setCurrentIndex(0);
                ui->SyncBox->blockSignals(false);
            }
        }
        auto newStartB = ui->StartSpinBox->value();
        auto newEndB = ui->WidthSpinBox->value() + newStartB;
        bool flag = config.setGateBEnd(newEndB);
        if (flag == false) {
            config.setGateBStart(newStartB);
            config.setGateBEnd(newEndB);
        }
        config.setGateBStart(newStartB);

    } else if (currentGate == GATE::GATE_C) {
        // index: 0=Pulser, 1=GateI, 2=GateA, 3=GateB
        if (index == 1 && config.getGateIEnable()) {
            config.setGateCSynchronMode(GateSynchron::GateI);
        } else if (index == 2 && config.getGateAEnable()) {
            config.setGateCSynchronMode(GateSynchron::GateA);
        } else if (index == 3 && config.getGateBEnable()) {
            config.setGateCSynchronMode(GateSynchron::GateB);
        } else {
            config.setGateCSynchronMode(GateSynchron::Pulser);
            if ((index == 1 && !config.getGateIEnable()) || (index == 2 && !config.getGateAEnable())
                || (index == 3 && !config.getGateBEnable())) {
                ui->SyncBox->blockSignals(true);
                ui->SyncBox->setCurrentIndex(0);
                ui->SyncBox->blockSignals(false);
            }
        }
        auto newStartC = ui->StartSpinBox->value();
        auto newEndC = ui->WidthSpinBox->value() + newStartC;
        bool flagC = config.setGateCEnd(newEndC);
        if (flagC == false) {
            config.setGateCStart(newStartC);
            config.setGateCEnd(newEndC);
        }
        config.setGateCStart(newStartC);
    }
    // 闸门A、B或C同步GateI时，闸门I自动设为波前模式
    if (config.getGateASynchronMode() == GateSynchron::GateI
        || config.getGateBSynchronMode() == GateSynchron::GateI
        || config.getGateCSynchronMode() == GateSynchron::GateI) {
        config.setGateIMeasureType(measureType::WaveFront);
    } else {
        config.setGateIMeasureType(measureType::MaxPeak);
    }

    if (config.getGateBSynchronMode() == GateSynchron::GateA
        || config.getGateCSynchronMode() == GateSynchron::GateA) {
        config.setGateAMeasureType(measureType::WaveFront);
    } else {
        config.setGateAMeasureType(measureType::MaxPeak);
    }
    if (config.getGateCSynchronMode() == GateSynchron::GateB) {
        config.setGateBMeasureType(measureType::WaveFront);
    } else if (config.getGateCSynchronMode() != GateSynchron::GateA) {
        // 只在没有其他闸门同步到GateB时恢复
        config.setGateBMeasureType(measureType::MaxPeak);
    }
    if (config.getGateCSynchronMode() == GateSynchron::GateA
        || config.getGateCSynchronMode() == GateSynchron::GateB) {
        // 闸门C同步到A或B时，目标闸门设为波前模式
        if (config.getGateCSynchronMode() == GateSynchron::GateA)
            config.setGateAMeasureType(measureType::WaveFront);
    }
    emit App::getInstance()->signal_showhideGatestatus();
    emit App::getInstance()->signal_GateView_Refresh();
}



void Gate_widget::on_MeasureBox_currentIndexChanged(int index)
{
    if (currentGate == GATE::GATE_I) {
        config.setGateIMeasureType(static_cast<measureType>(index));
    } else if (currentGate == GATE::GATE_A) {
        config.setGateAMeasureType(static_cast<measureType>(index));
    } else if (currentGate == GATE::GATE_B) {
        config.setGateBMeasureType(static_cast<measureType>(index));
    } else if (currentGate == GATE::GATE_C) {
        config.setGateCMeasureType(static_cast<measureType>(index));
    }
}

void Gate_widget::on_SynAcqisitBox_currentIndexChanged(int index)
{
    if (currentGate == GATE::GATE_I) {
        if (index) { // 开启同步采集
            config.setGateISyncSample(true);
        } else { // 关闭同步采集
            config.setGateISyncSample(false);
        }
    }
    emit App::getInstance()->signal_showhideGatestatus();
    emit App::getInstance()->signal_GateView_Refresh();
}

void Gate_widget::on_StartSpinBox_editingFinished()
{
    if (currentGate == GATE::GATE_I) {
        auto newStart = ui->StartSpinBox->value();
        auto newEnd = ui->WidthSpinBox->value() + newStart;

        bool flag = config.setGateIEnd(newEnd);
        if (flag == false) {
            config.setGateIStart(newStart);
            config.setGateIEnd(newEnd);
        }
        config.setGateIStart(newStart);

    } else if (currentGate == GATE::GATE_A) {
        auto newStart = ui->StartSpinBox->value();
        auto newEnd = ui->WidthSpinBox->value() + newStart;
        bool flag = config.setGateAEnd(newEnd);
        if (flag == false) {
            config.setGateAStart(newStart);
            config.setGateAEnd(newEnd);
        }
        config.setGateAStart(newStart);

    } else if (currentGate == GATE::GATE_B) {
        auto newStart = ui->StartSpinBox->value();
        auto newEnd = ui->WidthSpinBox->value() + newStart;
        bool flag = config.setGateBEnd(newEnd);
        if (flag == false) {
            config.setGateBStart(newStart);
            config.setGateBEnd(newEnd);
        }
        config.setGateBStart(newStart);

    } else if (currentGate == GATE::GATE_C) {
        auto newStart = ui->StartSpinBox->value();
        auto newEnd = ui->WidthSpinBox->value() + newStart;
        bool flag = config.setGateCEnd(newEnd);
        if (flag == false) {
            config.setGateCStart(newStart);
            config.setGateCEnd(newEnd);
        }
        config.setGateCStart(newStart);
    }

    emit App::getInstance()->signal_showhideGatestatus();
    emit App::getInstance()->signal_GateView_Refresh();
}

void Gate_widget::on_WidthSpinBox_editingFinished()
{
    if (currentGate == GATE::GATE_I) {
        config.setGateIEnd(ui->WidthSpinBox->value() + config.getGateIStart());
    } else if (currentGate == GATE::GATE_A) {
        config.setGateAEnd(ui->WidthSpinBox->value() + config.getGateAStart());
    } else if (currentGate == GATE::GATE_B) {
        config.setGateBEnd(ui->WidthSpinBox->value() + config.getGateBStart());
    } else if (currentGate == GATE::GATE_C) {
        config.setGateCEnd(ui->WidthSpinBox->value() + config.getGateCStart());
    }
    emit App::getInstance()->signal_showhideGatestatus();
    emit App::getInstance()->signal_GateView_Refresh();
}

void Gate_widget::on_ThresholdSpinBox_editingFinished()
{
    auto value = ui->ThresholdSpinBox->value();
    if (value <= 0) {
        value = 0;
        ui->ThresholdSpinBox->setValue(0);
    } else if (value > 100) {
        value = 100;
        ui->ThresholdSpinBox->setValue(100);
    }

    if (currentGate == GATE::GATE_I) {
        config.setGateIThreshold(value);
    } else if (currentGate == GATE::GATE_A) {
        config.setGateAThreshold(value);
    } else if (currentGate == GATE::GATE_B) {
        config.setGateBThreshold(value);
    } else if (currentGate == GATE::GATE_C) {
        config.setGateCThreshold(value);
    }

    if (currentGate == GATE::GATE_I) {
        ui->WidthSpinBox->setValue(config.getGateIEnd() - config.getGateIStart());
    } else if (currentGate == GATE::GATE_A) {
        ui->WidthSpinBox->setValue(config.getGateAEnd() - config.getGateAStart());
    } else if (currentGate == GATE::GATE_B) {
        ui->WidthSpinBox->setValue(config.getGateBEnd() - config.getGateBStart());
    } else if (currentGate == GATE::GATE_C) {
        ui->WidthSpinBox->setValue(config.getGateCEnd() - config.getGateCStart());
    }

    emit App::getInstance()->signal_showhideGatestatus();
}

void Gate_widget::on_StartSpinBox_valueChanged(double start)
{
    if (!isBlockSignal[ui->StartSpinBox]) {
        if (currentGate == GATE::GATE_I) {
            auto newEnd = ui->WidthSpinBox->value() + start;
            config.setGateIEnd(newEnd);
            config.setGateIStart(start);
        } else if (currentGate == GATE::GATE_A) {
            auto newEnd = ui->WidthSpinBox->value() + start;
            config.setGateAEnd(newEnd);
            config.setGateAStart(start);
        } else if (currentGate == GATE::GATE_B) {
            auto newEnd = ui->WidthSpinBox->value() + start;
            config.setGateBEnd(newEnd);
            config.setGateBStart(start);
        } else if (currentGate == GATE::GATE_C) {
            auto newEnd = ui->WidthSpinBox->value() + start;
            config.setGateCEnd(newEnd);
            config.setGateCStart(start);
        }
        emit App::getInstance()->signal_GateView_Refresh();
    }
}

void Gate_widget::on_WidthSpinBox_valueChanged(double width)
{
    if (!isBlockSignal[ui->WidthSpinBox]) {
        if (currentGate == GATE::GATE_I) {
            config.setGateIEnd(width + config.getGateIStart());
        } else if (currentGate == GATE::GATE_A) {
            config.setGateAEnd(width + config.getGateAStart());
        } else if (currentGate == GATE::GATE_B) {
            config.setGateBEnd(width + config.getGateBStart());
        } else if (currentGate == GATE::GATE_C) {
            config.setGateCEnd(width + config.getGateCStart());
        }
        emit App::getInstance()->signal_GateView_Refresh();
    }
}

void Gate_widget::on_ThresholdSpinBox_valueChanged(double threshold)
{
    if (!isBlockSignal[ui->ThresholdSpinBox]) {
        auto value = threshold;
        if (value <= 0) {
            value = 0;
            ui->ThresholdSpinBox->setValue(0);
        } else if (value > 100) {
            value = 100;
            ui->ThresholdSpinBox->setValue(100);
        }

        if (currentGate == GATE::GATE_I) {
            config.setGateIThreshold(value);
        } else if (currentGate == GATE::GATE_A) {
            config.setGateAThreshold(value);
        } else if (currentGate == GATE::GATE_B) {
            config.setGateBThreshold(value);
        } else if (currentGate == GATE::GATE_C) {
            config.setGateCThreshold(value);
        }
        emit App::getInstance()->signal_GateView_Refresh();

        if (currentGate == GATE::GATE_I) {
            ui->WidthSpinBox->setValue(config.getGateIEnd() - config.getGateIStart());
        } else if (currentGate == GATE::GATE_A) {
            ui->WidthSpinBox->setValue(config.getGateAEnd() - config.getGateAStart());
        } else if (currentGate == GATE::GATE_B) {
            ui->WidthSpinBox->setValue(config.getGateBEnd() - config.getGateBStart());
        } else if (currentGate == GATE::GATE_C) {
            ui->WidthSpinBox->setValue(config.getGateCEnd() - config.getGateCStart());
        }
    }
}
