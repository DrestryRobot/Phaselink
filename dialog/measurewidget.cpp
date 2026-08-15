#include "measurewidget.h"

#include <QDebug>
#include <QString>

#include "ui_measurewidget.h"

MeasureWidget::MeasureWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MeasureWidget)
    , config(Client::getInstance())
    , time(new QTimer(this))
{
    ui->setupUi(this);

    updateConfigCache();
    connect(time, &QTimer::timeout, this, &MeasureWidget::updateUI);
    time->start(400);
    connect(App::getInstance(), &App::signal_RangeChanged, this,
            &MeasureWidget::updateConfigCache);
    connect(App::getInstance(), &App::signal_AmplitudeChanged, this,
            &MeasureWidget::updateConfigCache);
    connect(App::getInstance(), &App::signal_GateView_Refresh, this,
            &MeasureWidget::updateConfigCache);
}

MeasureWidget::~MeasureWidget()
{
    if (time) {
        time->stop();
    }
    delete ui;
}

void MeasureWidget::updateConfigCache()
{
    range_start = config.getRangeStart();
    range_end = config.getRangeEnd();
    pointQuantity = config.getPointQuantity();
    reciprocalPointQuantity = (pointQuantity > 0) ? (1.0 / pointQuantity) : 0.001;
    maxAmp = static_cast<double>(config.getMaxAmplitude());
    gateA = config.getGateASynchronMode();
    gateB = config.getGateBSynchronMode();
    gateC = config.getGateCSynchronMode();

    auto groupNo = config.getGroupsNo();
    groupdata_size = 0;
    for (int i = 0; i < groupNo.size(); i++) {
        int id = groupNo[i];
        if (id == config.getCurrentGroup()) {
            break;
        }
        int pq = config.getPointQuantity(id);
        int bc = config.getBeamCounts(id);
        if (pq > 0 && bc > 0) {
            groupdata_size += (pq + 16) * bc;
        }
    }
}

void MeasureWidget::slot_Recive_date(const QByteArray &datapacket)
{
    if (datapacket.size() < static_cast<int>(sizeof(CScan_MeasureValue))) {
        return;
    }
    int beam = App::getInstance()->CurrentBeam;
    if (beam < 1) beam = 1;
    auto groupOffset = groupdata_size * static_cast<int>(sizeof(int16_t));
    auto tail = 16 * (beam - 1) * static_cast<int>(sizeof(int16_t));
    auto current = pointQuantity * beam * static_cast<int>(sizeof(int16_t));
    auto totalOffset = groupOffset + current + tail;
    if (totalOffset < 0 || totalOffset + static_cast<int>(sizeof(CScan_MeasureValue)) > datapacket.size()) {
        return;
    }

    CScan_MeasureValue localMeasure;
    memcpy(&localMeasure, datapacket.data() + totalOffset, sizeof(localMeasure));

    double localSA = ((localMeasure.gateAPositon_high * 256 + localMeasure.gateAPositon_low)
                 * (range_end - range_start))
            * reciprocalPointQuantity
        + range_start;

    double localSB =
        localMeasure.gateBPositon * (range_end - range_start) * reciprocalPointQuantity
        + range_start;

    double localSI =
        localMeasure.gateIPositon * (range_end - range_start) * reciprocalPointQuantity
        + range_start;

    double localSC =
        localMeasure.gateCPositon * (range_end - range_start) * reciprocalPointQuantity
        + range_start;

    // 写保护（处理器线程 -> 主线程 timer 读取）
    {
        QMutexLocker locker(&m_measureMutex);
        currentMeasureInfo = localMeasure;
        currentSA = localSA;
        currentSB = localSB;
        currentSC = localSC;
        currentSI = localSI;
    }

    App::getInstance()->SA = localSA;
    App::getInstance()->SB = localSB;
    App::getInstance()->SC = localSC;
    App::getInstance()->SI = localSI;
}

void MeasureWidget::updateUI()
{
    CScan_MeasureValue localMeasure;
    double localSA, localSB, localSC, localSI;
    {
        QMutexLocker locker(&m_measureMutex);
        localMeasure = currentMeasureInfo;
        localSA = currentSA;
        localSB = currentSB;
        localSC = currentSC;
        localSI = currentSI;
    }

    ui->label->setText(QString::number(
        static_cast<double>(localMeasure.gateAAmplitude) * 100.0 / maxAmp, 'f', 2));
    ui->label_2->setText(QString::number(
        static_cast<double>(localMeasure.gateBAmplitude) * 100.0 / maxAmp, 'f', 2));
    ui->label_4->setText(QString::number(
        static_cast<double>(localMeasure.gateIAmplitude) * 100.0 / maxAmp, 'f', 2));

    double cAmp = (static_cast<double>(localMeasure.gateCAmplitude_high) * 256.0);
    ui->label_7->setText(QString::number(cAmp * 100.0 / maxAmp, 'f', 2));

    ui->label_3->setText(QString::number(localSA, 'f', 2));
    ui->label_5->setText(QString::number(localSB, 'f', 2));
    ui->label_8->setText(QString::number(localSC, 'f', 2));
    ui->label_6->setText(QString::number(localSI, 'f', 2));
}
