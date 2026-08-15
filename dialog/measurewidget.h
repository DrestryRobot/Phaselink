#ifndef MEASUREWIDGET_H
#define MEASUREWIDGET_H

#include <app.h>
#include <client.h>

#include <QMutex>
#include <QTimer>
#include <QWidget>
#include <cstdint>
namespace Ui
{
    class MeasureWidget;
}

class MeasureWidget : public QWidget
{
    Q_OBJECT

    // 测量值结构体
    struct CScan_MeasureValue
    {
        uint64_t gateBAmplitude : 16;
        uint64_t gateBPositon : 24;
        uint64_t gateAAmplitude : 16;
        uint64_t gateAPositon_low : 8;

        uint64_t gateAPositon_high : 16;
        uint64_t gateIAmplitude : 16;
        uint64_t gateIPositon : 16;
        uint64_t gateIMaxPositon : 16;

        uint64_t gateCAmplitude_high : 8;
        uint64_t gateCPositon : 24;
        uint64_t gateDAmplitude : 16;
        uint64_t gateDPositon_low : 8;

        uint64_t gateDPositon_high : 16;
        uint64_t gateEAmplitude : 16;
        uint64_t gateEPositon : 24;
        uint64_t reverse : 8;
    };

public:
    explicit MeasureWidget(QWidget* parent = nullptr);
    ~MeasureWidget();

    Q_INVOKABLE void slot_Recive_date(const QByteArray& datapacket);

    void updateConfigCache();
private slots:
    void updateUI();

private:
    Ui::MeasureWidget* ui;
    Client& config;
    QTimer* time;
    mutable QMutex m_measureMutex;
    CScan_MeasureValue currentMeasureInfo;
    double currentSA = 0.0;
    double currentSB = 0.0;
    double currentSC = 0.0;
    double currentSI = 0.0;
    double maxAmp = 4096;

    double range_start = 0.0;
    double range_end = 50.0;
    int pointQuantity = 1024;
    double reciprocalPointQuantity = 0.0;
    int groupdata_size = 0;
    GateSynchron gateA = GateSynchron::GateI;
    GateSynchron gateB = GateSynchron::GateI;
    GateSynchron gateC = GateSynchron::GateI;
};

#endif  // MEASUREWIDGET_H
