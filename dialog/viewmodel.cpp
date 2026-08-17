#include "viewmodel.h"
#include "client.h"
#include "dialog/viewwidget.h"
#include "3DScan/scandata.h"

#include <QDebug>
#include <algorithm>
#include <cstring>
#include <vector>

extern double Thick;

ViewModel::ViewModel(QWidget *parent)
    : QWidget(parent)
{
}

ViewModel::~ViewModel()
{
}

void ViewModel::slot_Recive_date(int beam, const QByteArray &datapacket,
                                 bool Replay, int No)
{
    Q_UNUSED(Replay);
    Q_UNUSED(No);
    extractAndPushDefectCloudData(beam, datapacket);
}

void ViewModel::extractAndPushDefectCloudData(int beam_0, const QByteArray &datapacket)
{
    if (datapacket.isEmpty()) {
        return;
    }

    // 获取配置参数
    Client &config = Client::getInstance();
    int pointQuantity = config.getPointQuantity();
    double i_threshold = config.getGateIThreshold() / 100;
    double a_threshold = config.getGateAThreshold() / 100;
    double b_threshold = config.getGateBThreshold() / 100;
    double maxAmp = static_cast<double>(config.getMaxAmplitude());

    // 检查数据包大小（+sizeof(CScan_MeasureValue) 确保最后一波束元数据不越界）
    int expectedSize = beam_0 * (pointQuantity + 16) * sizeof(int16_t)
                       + static_cast<int>(sizeof(CScan_MeasureValue));
    if (datapacket.size() < expectedSize) {
        qWarning() << "Ultrasound data: Data packet size too small:"
                   << datapacket.size() << "expected:" << expectedSize;
        return;
    }

    double range_start = config.getRangeStart();
    double range_end = config.getRangeEnd();
    double reciprocalPointQuantity = 1.0 / pointQuantity;
    constexpr int kBoundaryWin = 5;   // 居中窗口：输出帧 ±2 帧

    double curAmp[64] = { 0.0 };
    double curTof[64] = { 0.0 };
    double curSi[64] = { 0.0 };
    bool valid[64] = { false };

    // 第一遍：逐波束解析测量值，做门幅值/位置/TOF/有效性处理
    for (int i = 0; i < beam_0; i++) {
        CScan_MeasureValue measure;
        auto current = (pointQuantity * (i + 1) + 16 * i) * sizeof(int16_t);
        memcpy(&measure, datapacket.data() + current, sizeof(measure));

        double gateAmp = static_cast<double>(measure.gateAAmplitude) / maxAmp;
        double gateBAmp = static_cast<double>(measure.gateBAmplitude) / maxAmp;
        double gateIAmp = static_cast<double>(measure.gateIAmplitude) / maxAmp;

        double sa = (measure.gateAPositon_high * 256 + measure.gateAPositon_low)
                    * (range_end - range_start) * reciprocalPointQuantity + range_start;
        double sb = measure.gateBPositon * (range_end - range_start)
                    * reciprocalPointQuantity + range_start;
        double siPos = measure.gateIPositon * (range_end - range_start)
                       * reciprocalPointQuantity + range_start;

        // 缺陷判断：A 门有波且 B 门无波 → 用 A 门位置，否则用 B 门位置
        double tofVal = (gateAmp > a_threshold && gateBAmp < b_threshold)
                        ? (sa - siPos) / Thick
                        : (sb - siPos) / Thick;

        // 有效性：I 门幅值代表是否在板子上（原始判定，不随时间滞回；
        // 抗抖动交给下方居中窗口的中值边界处理）
        valid[i] = (gateIAmp > i_threshold);

        curAmp[i] = gateAmp;
        curTof[i] = tofVal;
        curSi[i] = siPos;
    }

    // ============================================================
    // 点都补齐（不做板内板外判断，与最早版一致）：
    //   - I 门有效：写原始值；
    //   - 无效的边缘波束（0/末）：也写原始值；
    //   - 无效的非边缘波束：不覆盖，保持上一次的值（补齐，内部不留空缺）。
    // ============================================================
    for (int i = 0; i < beam_0; i++) {
        bool isEdge = (i == 0 || i == beam_0 - 1);
        if (valid[i]) {
            amp[i] = curAmp[i];
            tof[i] = curTof[i];
            if (i == (beam_0 - 1) / 2) si = curSi[i];
            beamValid[i] = true;
        } else if (isEdge) {
            amp[i] = curAmp[i];
            tof[i] = curTof[i];
            beamValid[i] = true;
        } else {
            // 非边缘无效：保持上一次的 amp/tof（不覆盖即保留），点都补齐
            beamValid[i] = true;
        }
    }
    beam = beam_0;
}
