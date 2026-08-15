#include "viewmodel.h"
#include <QDebug>
#include <kuka3denums.h>
#include <libkuka3d.h>
#include "3DScan/scandata.h"
#include <vector>
#include <algorithm>

extern double Thick;

double sumSi = 0.0;
int validBeamCount = 0;

ViewModel::ViewModel(QWidget *parent)
    : QWidget(parent)
    , m_is3DInitialized(false)
    , m_frameCount(0)
{
    init3DView();

    m_robotTimer = new QTimer(this);
    connect(m_robotTimer, &QTimer::timeout, this, [this]() {
        int beam_0 = 49;
        QByteArray datapacket;

        extractAndPushDefectCloudData(beam_0, datapacket);
    });

    m_robotTimer->start(8);
}

ViewModel::~ViewModel()
{

}

void ViewModel::init3DView()
{
    m_libKuka3D = Kuka3D::LibKuka3D::getInstance();

    // 设置机器人位置类型为极坐标
    m_libKuka3D->setRobotPositionType(Kuka3D::RobotPositionType_XYZABC);

    // 设置机器人模型
    m_libKuka3D->setRobotVisible(true);

    // 设置探头尺寸
    m_libKuka3D->setProbeSize(23.0, 16.0, 25.0);
    QWidget * t = m_libKuka3D->getKuka3DView();

    // m_libKuka3D->setToolMatTranslate(
    //     -1, 0, 0, 0,   // 第一行
    //     0, -1, 0, 0,   // 第二行
    //     0, 0, 1, 0     // 第三行
    //     );

    // m_libKuka3D->setAsyncMode(Kuka3D::CSCAN_3D,false); //同步模式

    QHBoxLayout* hlayout = new QHBoxLayout;
    hlayout->addWidget(t);
    hlayout->setContentsMargins(0, 0, 0, 0);

    this->setLayout(hlayout);

    m_is3DInitialized = true;
}

void ViewModel::slot_Recive_date(int beam, const QByteArray &datapacket,
                                 bool Replay, int No)
{
    if (m_is3DInitialized) {
        extractAndPushDefectCloudData(beam, datapacket);
    }
}

void ViewModel::extractAndPushDefectCloudData(int beam_0, const QByteArray &datapacket)
{
    if (!m_libKuka3D || datapacket.isEmpty()) {
        return;
    }

    // uint64_t timestamp = getCurrentTimerStamp();

    // 获取配置参数
    static uint64_t tt = 4000;
    auto t0 = std::chrono::steady_clock::now();
    // auto t1 = t0 + std::chrono::microseconds(scanData->time_stamp);
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                             t0.time_since_epoch()
                             ).count();


    // 获取配置参数
    tt += 5000;

    // // 模拟超声数据
    // int pointQuantity = 1024;
    // int beam = 49;

    // QVector<uint8_t> fullAmpData;

    // Kuka3D::ScanHeaderFromSdk header;

    // header.height = 40000;
    // header.beamCnt = beam;
    // header.beamLength = pointQuantity;
    // header.width = 28800;

    // std::vector< Kuka3D::GateInfo *> gates;

    // for(auto i = 0 ; i < beam; i++) {
    //     Kuka3D::GateInfo *gate = new Kuka3D::GateInfo;

    //     gate->SI = 2;
    //     gate->A_AMP = 1;
    //     gate->A_TOF = 1;
    //     gate->DATA_VALID = 1;
    //     gates.push_back(gate);
    // }


    // 真实超声数据
    Client &config = Client::getInstance();
    int pointQuantity = config.getPointQuantity();
    double i_threshold = config.getGateIThreshold() / 100;
    double a_threshold = config.getGateAThreshold() / 100;
    double b_threshold = config.getGateBThreshold() / 100;

    // 检查数据包大小（+sizeof(CScan_MeasureValue) 确保最后一波束元数据不越界）
    int expectedSize = beam_0 * (pointQuantity + 16) * sizeof(int16_t) + static_cast<int>(sizeof(CScan_MeasureValue));
    if (datapacket.size() < expectedSize) {
        qWarning() << "Defect cloud: Data packet size too small:" << datapacket.size() << "expected:" << expectedSize;
        return;
    }

    // 提取完整的A-Scan数据（所有波束的所有点）
    QVector<uint8_t> fullAmpData;
    fullAmpData.reserve(beam_0 * pointQuantity);

    const int16_t *rawData = reinterpret_cast<const int16_t*>(datapacket.data());
    double maxAmp =  static_cast<double>(config.getMaxAmplitude()); // 最大幅值
    // 提取每个波束的所有点的数据
    for (int i = 0; i < beam_0; i++) {
        // 每个波束的数据起始位置
        int beamOffset = i * (pointQuantity + 16);

        // 提取该波束的所有点
        for (int j = 0; j < pointQuantity; j++) {
            int dataIndex = beamOffset + j;
            if (dataIndex < datapacket.size() / sizeof(int16_t)) {
                int16_t rawValue = rawData[dataIndex];

                // 转换为0-255的范围
                double percentage = std::abs(rawValue) * 100.0 / maxAmp;
                int colorIndex = static_cast<int>(percentage * 2.55);
                colorIndex = qBound(0, colorIndex, 255);

                fullAmpData.append(static_cast<uint8_t>(colorIndex));
            } else {
                fullAmpData.append(0);
            }
        }
    }

    // 准备ScanHeaderFromSdk结构
    Kuka3D::ScanHeaderFromSdk header;

    header.beamCnt = beam_0;
    header.beamLength = pointQuantity;  // 设置每个波束的点数
    std::vector< Kuka3D::GateInfo *> gates;
    // 高度为扫描范围
    double range_start = config.getRangeStart();
    double range_end = config.getRangeEnd();
    header.height = static_cast<int32_t>((range_end - range_start) * 1000); // 转为0.001mm

    // 计算扫描宽度
    auto num = (config.getBeamLastElement() - config.getBeamFirstElement()
                - config.getBeamAperture() + 1) / config.getBeamElementStep() + 1;

    double scanWidth = (num - 1) * config.getProbePrimaryElementsPitch()
                       * config.getBeamElementStep();
    header.width = static_cast<int32_t>(scanWidth * 1000); // 转为0.001mm

    CScan_MeasureValue currentMeasureInfo;
    double reciprocalPointQuantity = 1.0 / pointQuantity;
    bool valid[64] = { false };
    constexpr int kInvalidConfirmFrames = 3;
    constexpr int kBoundaryWin = 5;

    for(auto i = 0 ; i < beam_0; i++)
    {
        Kuka3D::GateInfo *gate = new Kuka3D::GateInfo;

        auto tail = 16 * (i) * sizeof(int16_t);
        auto current = pointQuantity * (i+1) * sizeof(int16_t);
        memcpy(&currentMeasureInfo, datapacket.data() + current + tail, sizeof(currentMeasureInfo));

        gate->A_AMP  =  static_cast<double>(currentMeasureInfo.gateAAmplitude)  / maxAmp;

        gate->B_AMP  =  static_cast<double>(currentMeasureInfo.gateBAmplitude)  / maxAmp;

        gate->I_AMP   = static_cast<double>(currentMeasureInfo.gateIAmplitude)  / maxAmp;

        gate->SA = ((currentMeasureInfo.gateAPositon_high * 256 + currentMeasureInfo.gateAPositon_low) * (range_end - range_start)) * reciprocalPointQuantity + range_start;

        gate->SB = currentMeasureInfo.gateBPositon * (range_end - range_start) * reciprocalPointQuantity + range_start;

        gate->SI = currentMeasureInfo.gateIPositon * (range_end - range_start) * reciprocalPointQuantity + range_start;

        // 缺陷判断
        if(gate->A_AMP > a_threshold && gate->B_AMP < b_threshold)
        {
            gate->A_TOF = (gate->SA - gate->SI) / Thick;
        }
        else
        {
            gate->A_TOF = (gate->SB - gate->SI) / Thick;
        }

        // amp[i] = gate->A_AMP;

        // tof[i] = gate->A_TOF;

        // if(i == (beam_0-1)/2) si = gate->SI;

        // beam = beam_0;

        // // 有效波形
        // if(gate->I_AMP > i_threshold)
        // {
        //     gate->DATA_VALID = 1;

        //     sumSi += gate->SI;

        //     validBeamCount++;
        // }
        // else
        // {
        //     gate->DATA_VALID = 0;

        //     amp[i] = 0.0;

        //     tof[i] = 0.0;

        //     si = 0.0;
        // }


        // 先记录每个波束本帧是否有效（I 门幅值代表是否在板子上）
        valid[i] = (gate->I_AMP > i_threshold);
        // 时间滞回：有效立即恢复；无效需连续多帧确认，避免偶发未耦合被误判为板外
        if (valid[i]) {
            m_invalidRun[i] = 0;
            m_smoothValid[i] = true;
        } else if (++m_invalidRun[i] >= kInvalidConfirmFrames) {
            m_smoothValid[i] = false;
        }

        gates.push_back(gate);
    }

    // 用当前帧有效波束的连续区间判断板内/板外：
    // 区间内 = 板内（空洞用上一帧补），区间外 = 板外（过滤）
    int rawFirst = -1;
    int rawLast = -1;
    for (int i = 0; i < beam_0; i++) {
        if (m_smoothValid[i]) { rawFirst = i; break; }
    }
    for (int i = beam_0 - 1; i >= 0; i--) {
        if (m_smoothValid[i]) { rawLast = i; break; }
    }

    // 沿前进方向（时间轴）对边界做中值平滑，抑制边缘波束阈值抖动造成的锯齿
    if (rawFirst >= 0) {
        m_firstHist.push_back(rawFirst);
        m_lastHist.push_back(rawLast);
        while ((int)m_firstHist.size() > kBoundaryWin) m_firstHist.pop_front();
        while ((int)m_lastHist.size() > kBoundaryWin) m_lastHist.pop_front();
    }

    int firstValid = -1;
    int lastValid = -1;
    if (!m_firstHist.empty()) {
        std::vector<int> fv(m_firstHist.begin(), m_firstHist.end());
        std::vector<int> lv(m_lastHist.begin(), m_lastHist.end());
        std::sort(fv.begin(), fv.end());
        std::sort(lv.begin(), lv.end());
        firstValid = fv[fv.size() / 2];
        lastValid = lv[lv.size() / 2];
    }

    // 第二遍：按区间填充并同步 beamValid
    for (int i = 0; i < beam_0; i++) {
        Kuka3D::GateInfo *gate = gates[i];
        bool inside = (firstValid >= 0 && i >= firstValid && i <= lastValid);
        if (valid[i]) {
            amp[i] = gate->A_AMP;
            tof[i] = gate->A_TOF;
            if (i == (beam_0 - 1) / 2) si = gate->SI;
            gate->DATA_VALID = 1;
            beamValid[i] = true;
        } else if (inside && m_smoothValid[i]) {
            // 板内偶发未耦合：全局量和 gate 都用上一帧补
            gate->A_AMP = amp[i];
            gate->A_TOF = tof[i];
            gate->DATA_VALID = 1;
            beamValid[i] = true;
        } else {
            // 板外：过滤
            amp[i] = 0.0;
            tof[i] = 0.0;
            gate->DATA_VALID = 0;
            beamValid[i] = false;
        }
    }

    beam = beam_0;

    m_libKuka3D->addOneFrameData(header, gates, fullAmpData.data(), timestamp,tt);

}
