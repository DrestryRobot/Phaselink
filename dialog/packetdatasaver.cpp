#include "PacketDataSaver.h"

#include <QDataStream>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>

#include "qdatetime.h"
#include "qdebug.h"

// 文件格式标识
static const char FILE_MAGIC[] = "PHASELINK_DATA_V3";
static const int MAGIC_LEN = 16;
// 配置版本：用于向后兼容，新增字段时递增
static const quint32 CONFIG_VERSION = 3;

PacketDataSaver::PacketDataSaver(QObject *parent)
    : QObject(parent)
    , config(Client::getInstance())
{
}

void PacketDataSaver::setData(const QMap<quint32, QByteArray> &data)
{
    m_packetMap = data;
}

const QMap<quint32, QByteArray> &PacketDataSaver::getData() const
{
    return m_packetMap;
}

bool PacketDataSaver::saveToFile(int num, int points, QWidget *parent)
{
    QString fileName = QFileDialog::getSaveFileName(
        parent, tr("Save Scan Data"),
        QDir::homePath() + QString("/scan_data_%1.pdata")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        tr("Phaselink Data (*.pdata);;All Files (*)"));

    if (fileName.isEmpty()) {
        emit operationCompleted(false, tr("Save canceled"));
        return false;
    }

    return saveToFile(fileName, num, points);
}

bool PacketDataSaver::saveToFile(const QString &filename, int num, int points)
{
    QString actualFilename = filename;
    if (!actualFilename.endsWith(".pdata", Qt::CaseInsensitive)) {
        actualFilename += ".pdata";
    }

    QFile file(actualFilename);
    if (!file.open(QIODevice::WriteOnly)) {
        emit operationCompleted(false, tr("Failed to open file for writing"));
        return false;
    }

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_6_0);
    out.setByteOrder(QDataStream::LittleEndian);

    // 1. 写入文件头标识
    out.writeRawData(FILE_MAGIC, MAGIC_LEN);
    out << CONFIG_VERSION;

    // 2. 收集并写入配置参数
    auto &sc = m_scanConfig;
    sc.groupId = config.getCurrentGroup();

    // Epage
    sc.gain = config.getGain();
    sc.rangeStart = config.getRangeStart();
    sc.rangeEnd = config.getRangeEnd();
    sc.pitch = config.getProbePrimaryElementsPitch();
    sc.frequency = config.getProbeFrequency();
    sc.wedgeVelocity = config.getWedgeVelocity();
    sc.wedgeEnable = config.getWedgeEnable();
    sc.wedgeX = config.getWedgeX();
    sc.wedgeZ = config.getWedgeZ();
    sc.frameRate = config.getFrameRate();
    sc.voltage = config.getPaVoltage();
    sc.pulseWidth = config.getPulseWidth();
    sc.voltagePolarity = static_cast<int>(config.getVoltagePolarity());
    sc.filterHigh = config.getFilterHigh();
    sc.filterLow = config.getFilterLow();
    sc.rectifierMode = static_cast<int>(config.getRectifierMode());
    sc.maxAmplitude = static_cast<int>(config.getMaxAmplitude());
    sc.points = config.getPointQuantity();
    sc.zeroPosition = config.getTfmRecDelay();

    // Phasepage
    sc.scanType = static_cast<int>(config.getScanType());
    sc.beamMode = static_cast<int>(config.getBeamMode());
    sc.aperture = config.getBeamAperture();
    sc.elementStep = config.getBeamElementStep();
    sc.firstElement = config.getBeamFirstElement();
    sc.lastElement = config.getBeamLastElement();
    sc.angle = config.getBeamAngleStep();
    sc.focusMode = static_cast<int>(config.getFocusMode());
    sc.focalPosition = config.getFocalPosition();
    sc.transmission = config.getTransmissionChannel();
    sc.reception = config.getReceptionChannel();

    // GatePage
    sc.gateAEnable = config.getGateAEnable();
    sc.gateASync = static_cast<int>(config.getGateASynchronMode());
    sc.gateAStart = config.getGateAStart();
    sc.gateAEnd = config.getGateAEnd();
    sc.gateAThreshold = config.getGateAThreshold();
    sc.gateAMeasureType = static_cast<int>(config.getGateAMeasureType());
    sc.gateBEnable = config.getGateBEnable();
    sc.gateBSync = static_cast<int>(config.getGateBSynchronMode());
    sc.gateBStart = config.getGateBStart();
    sc.gateBEnd = config.getGateBEnd();
    sc.gateBThreshold = config.getGateBThreshold();
    sc.gateBMeasureType = static_cast<int>(config.getGateBMeasureType());
    sc.gateIEnable = config.getGateIEnable();
    sc.gateIStart = config.getGateIStart();
    sc.gateIEnd = config.getGateIEnd();
    sc.gateIThreshold = config.getGateIThreshold();
    sc.gateIMeasureType = static_cast<int>(config.getGateIMeasureType());
    sc.gateISyncSample = config.getGateISyncSample();
    sc.gateCEnable = config.getGateCEnable();
    sc.gateCSync = static_cast<int>(config.getGateCSynchronMode());
    sc.gateCStart = config.getGateCStart();
    sc.gateCEnd = config.getGateCEnd();
    sc.gateCThreshold = config.getGateCThreshold();
    sc.gateCMeasureType = static_cast<int>(config.getGateCMeasureType());

    // TcgPage
    sc.tcgEnable = config.getTcgEnable();
    sc.tcgDepths.clear();
    sc.tcgGains.clear();
    for (int i = 1;; i++) {
        double depth = config.getTcgPointDepthIndex(i);
        double gain = config.getTcgPointGainIndex(i);
        if (depth < 0.0)
            break;
        sc.tcgDepths.push_back(depth);
        sc.tcgGains.push_back(gain);
    }
    sc.tcgNums = sc.tcgDepths.size();
    sc.beamTcgData.clear();
    int beamCount = qMax(1, config.getBeamCounts());
    for (int b = 0; b < beamCount; b++) {
        QVector<double> beamPts;
        for (int i = 1;; i++) {
            double depth = config.getTcgPointDepth(i, b);
            double gain = config.getTcgPointGain(i, b);
            if (depth < 0.0)
                break;
            beamPts.push_back(depth);
            beamPts.push_back(gain);
        }
        sc.beamTcgData.push_back(beamPts);
    }
    sc.beamTcgCount = sc.beamTcgData.size();

    // ScanPage
    sc.resolution = App::getInstance()->currentPosition;
    sc.scanStart = App::getInstance()->ScanStart;
    sc.scanEnd = App::getInstance()->ScanEnd;
    sc.encoderDir = App::getInstance()->xNormal;
    sc.xResolution = App::getInstance()->resolutionEcoder1;
    sc.xPosition = App::getInstance()->position;
    sc.soundStart = App::getInstance()->SoundStart;
    sc.soundEnd = App::getInstance()->SoundEnd;
    sc.c1Data = static_cast<int>(App::getInstance()->C1);
    sc.c2Data = static_cast<int>(App::getInstance()->C2);

    // 编码器/时间模式参数
    sc.encoderIndex = App::getInstance()->EncoderIndex;
    sc.encoderSwapped = App::getInstance()->EncoderSwapped ? 1 : 0;
    sc.yResolution = App::getInstance()->resolutionEcoder2;
    sc.yNormal = App::getInstance()->yNormal;
    sc.scanResolution = App::getInstance()->scanResolution;
    sc.endPosition = App::getInstance()->End;
    sc.timeResolution = App::getInstance()->timeResolution;
    sc.scanLength = App::getInstance()->Scanlength;
    sc.encoderTimeMode = App::getInstance()->EncoderTimeMode;

    // Data info
    sc.beamCount = num;
    sc.pointCount = points;

    // ---- 写入二进制流 ----
    out << sc.groupId;

    out << sc.gain << sc.rangeStart << sc.rangeEnd;
    out << sc.pitch << sc.frequency;
    out << sc.longitudinalVelocity << sc.wedgeVelocity;
    out << sc.wedgeEnable << sc.wedgeX << sc.wedgeZ;
    out << sc.frameRate;
    out << sc.voltage << sc.pulseWidth << sc.voltagePolarity;
    out << sc.filterHigh << sc.filterLow;
    out << sc.rectifierMode << sc.maxAmplitude;
    out << sc.points << sc.zeroPosition;

    out << sc.scanType << sc.waveType << sc.beamMode;
    out << sc.aperture << sc.elementStep;
    out << sc.firstElement << sc.lastElement;
    out << sc.angle << sc.focusMode << sc.focalPosition;
    out << sc.transmission << sc.reception;

    out << sc.gateAEnable << sc.gateASync;
    out << sc.gateAStart << sc.gateAEnd << sc.gateAThreshold;
    out << sc.gateAMeasureType;
    out << sc.gateBEnable << sc.gateBSync;
    out << sc.gateBStart << sc.gateBEnd << sc.gateBThreshold;
    out << sc.gateBMeasureType;
    out << sc.gateIEnable;
    out << sc.gateIStart << sc.gateIEnd << sc.gateIThreshold;
    out << sc.gateIMeasureType << sc.gateISyncSample;

    out << sc.gateCEnable << sc.gateCSync;
    out << sc.gateCStart << sc.gateCEnd << sc.gateCThreshold;
    out << sc.gateCMeasureType;

    out << sc.tcgEnable << sc.tcgNums;
    for (int i = 0; i < sc.tcgNums; i++) {
        out << sc.tcgDepths[i] << sc.tcgGains[i];
    }
    out << sc.beamTcgCount;
    for (auto &beamPts : sc.beamTcgData) {
        int count = beamPts.size() / 2;
        out << count;
        for (double v : beamPts)
            out << v;
    }

    out << sc.resolution << sc.scanStart << sc.scanEnd;
    out << sc.encoderDir << sc.xResolution << sc.xPosition;
    out << sc.soundStart << sc.soundEnd;
    out << sc.c1Data << sc.c2Data;

    // 编码器/时间模式参数
    out << sc.encoderIndex << sc.encoderSwapped << sc.yResolution;
    out << sc.yNormal << sc.scanResolution << sc.endPosition;
    out << sc.timeResolution << sc.scanLength << sc.encoderTimeMode;

    out << sc.beamCount << sc.pointCount;

    // 3. 写入帧数据
    quint32 frameCount = static_cast<quint32>(m_packetMap.size());
    out << frameCount;

    for (auto it = m_packetMap.constBegin(); it != m_packetMap.constEnd(); ++it) {
        out << it.key();                                    // frameNo (uint32)
        out << static_cast<quint32>(it.value().size());     // dataLen (uint32)
        out.writeRawData(it.value().constData(), it.value().size()); // data
    }

    file.close();
    emit operationCompleted(true, tr("Data saved successfully to %1").arg(actualFilename));
    return true;
}

bool PacketDataSaver::loadFromFile(QWidget *parent)
{
    QString fileName = QFileDialog::getOpenFileName(
        parent, tr("Load Scan Data"), QDir::homePath(),
        tr("Phaselink Data (*.pdata);;All Files (*)"));

    if (fileName.isEmpty()) {
        emit operationCompleted(false, tr("Load canceled"));
        return false;
    }

    return loadFromFile(fileName);
}

bool PacketDataSaver::loadFromFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        emit operationCompleted(false, tr("Failed to open file for reading"));
        return false;
    }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_0);
    in.setByteOrder(QDataStream::LittleEndian);

    // 1. 校验文件头标识
    char magic[MAGIC_LEN + 1] = {0};
    if (in.readRawData(magic, MAGIC_LEN) != MAGIC_LEN || memcmp(magic, FILE_MAGIC, MAGIC_LEN) != 0) {
        emit operationCompleted(false, tr("Invalid file format (expected V3)"));
        return false;
    }

    quint32 fileConfigVersion = 0;
    in >> fileConfigVersion;

    // 2. 读取配置参数
    auto &sc = m_scanConfig;

    in >> sc.groupId;

    in >> sc.gain >> sc.rangeStart >> sc.rangeEnd;
    in >> sc.pitch >> sc.frequency;
    in >> sc.longitudinalVelocity >> sc.wedgeVelocity;
    in >> sc.wedgeEnable >> sc.wedgeX >> sc.wedgeZ;
    in >> sc.frameRate;
    in >> sc.voltage >> sc.pulseWidth >> sc.voltagePolarity;
    in >> sc.filterHigh >> sc.filterLow;
    in >> sc.rectifierMode >> sc.maxAmplitude;
    in >> sc.points >> sc.zeroPosition;

    in >> sc.scanType >> sc.waveType >> sc.beamMode;
    in >> sc.aperture >> sc.elementStep;
    in >> sc.firstElement >> sc.lastElement;
    in >> sc.angle >> sc.focusMode >> sc.focalPosition;
    in >> sc.transmission >> sc.reception;

    in >> sc.gateAEnable >> sc.gateASync;
    in >> sc.gateAStart >> sc.gateAEnd >> sc.gateAThreshold;
    in >> sc.gateAMeasureType;
    in >> sc.gateBEnable >> sc.gateBSync;
    in >> sc.gateBStart >> sc.gateBEnd >> sc.gateBThreshold;
    in >> sc.gateBMeasureType;
    in >> sc.gateIEnable;
    in >> sc.gateIStart >> sc.gateIEnd >> sc.gateIThreshold;
    in >> sc.gateIMeasureType >> sc.gateISyncSample;

    if (fileConfigVersion >= 2) {
        in >> sc.gateCEnable >> sc.gateCSync;
        in >> sc.gateCStart >> sc.gateCEnd >> sc.gateCThreshold;
        in >> sc.gateCMeasureType;
    } else {
        sc.gateCEnable = 0;
        sc.gateCSync = 0;
        sc.gateCStart = 0.0;
        sc.gateCEnd = 20.0;
        sc.gateCThreshold = 50.0;
        sc.gateCMeasureType = 0;
    }

    in >> sc.tcgEnable >> sc.tcgNums;
    sc.tcgDepths.clear();
    sc.tcgGains.clear();
    for (int i = 0; i < sc.tcgNums; i++) {
        double depth, gain;
        in >> depth >> gain;
        sc.tcgDepths.push_back(depth);
        sc.tcgGains.push_back(gain);
    }

    if (fileConfigVersion >= 2) {
        in >> sc.beamTcgCount;
        sc.beamTcgData.clear();
        for (int b = 0; b < sc.beamTcgCount; b++) {
            int count;
            in >> count;
            QVector<double> beamPts;
            for (int j = 0; j < count * 2; j++) {
                double v;
                in >> v;
                beamPts.push_back(v);
            }
            sc.beamTcgData.push_back(beamPts);
        }
    } else {
        sc.beamTcgCount = 0;
        sc.beamTcgData.clear();
    }

    in >> sc.resolution >> sc.scanStart >> sc.scanEnd;
    in >> sc.encoderDir >> sc.xResolution >> sc.xPosition;
    in >> sc.soundStart >> sc.soundEnd;
    in >> sc.c1Data >> sc.c2Data;

    // 编码器/时间模式参数
    in >> sc.encoderIndex >> sc.encoderSwapped >> sc.yResolution;
    in >> sc.yNormal >> sc.scanResolution >> sc.endPosition;
    in >> sc.timeResolution >> sc.scanLength >> sc.encoderTimeMode;

    in >> sc.beamCount >> sc.pointCount;

    // 恢复配置到Client和App
    applyConfigToClient();

    // 3. 读取帧数据
    m_packetMap.clear();
    quint32 frameCount;
    in >> frameCount;

    qint64 fileRemaining = file.size() - file.pos();

    for (quint32 i = 0; i < frameCount; i++) {
        quint32 frameNo;
        quint32 dataLen;
        in >> frameNo >> dataLen;

        if (dataLen == 0 || dataLen > 100 * 1024 * 1024) {
            emit operationCompleted(false, tr("Invalid data length at frame %1").arg(i));
            return false;
        }

        int dataLenInt = static_cast<int>(dataLen);
        if (dataLenInt + static_cast<int>(sizeof(frameNo) + sizeof(dataLen)) > fileRemaining) {
            emit operationCompleted(false, tr("File truncated at frame %1").arg(i));
            return false;
        }

        QByteArray data(dataLenInt, Qt::Uninitialized);
        if (in.readRawData(data.data(), dataLenInt) != dataLenInt) {
            emit operationCompleted(false, tr("File read error at frame %1").arg(i));
            return false;
        }

        fileRemaining -= (dataLenInt + sizeof(frameNo) + sizeof(dataLen));
        m_packetMap.insert(frameNo, data);
    }

    file.close();
    emit operationCompleted(true,
                            tr("Successfully loaded %1 frames from %2").arg(frameCount).arg(filename));
    return true;
}

void PacketDataSaver::applyConfigToClient()
{
    auto &sc = m_scanConfig;

    // Epage
    config.setCurrentGroup(sc.groupId);
    config.setGain(sc.gain);
    config.setRangeStart(sc.rangeStart);
    config.setRangeEnd(sc.rangeEnd);
    config.setProbePrimaryElementsPitch(sc.pitch);
    config.setProbeFrequency(sc.frequency);
    config.setWedgeVelocity(sc.wedgeVelocity);
    config.setWedgeEnable(sc.wedgeEnable);
    config.setWedgeX(sc.wedgeX);
    config.setWedgeZ(sc.wedgeZ);
    config.setFrameRate(sc.frameRate);
    config.setPaVoltage(sc.voltage);
    config.setPulseWidth(sc.pulseWidth);
    config.setVoltagePolarity(static_cast<PolarityType>(sc.voltagePolarity));
    config.setFilterHigh(sc.filterHigh);
    config.setFilterLow(sc.filterLow);
    config.setRectifierMode(static_cast<RectifierType>(sc.rectifierMode));
    config.setMaxAmplitude(static_cast<MaxAmplitude>(sc.maxAmplitude));
    config.setPointQuantity(sc.points);
    config.setTfmRecDelay(sc.zeroPosition);

    // Phasepage
    config.setScanType(static_cast<scanType>(sc.scanType));
    config.setBeamMode(static_cast<BeamMode>(sc.beamMode));
    config.setBeamAperture(sc.aperture);
    config.setBeamElementStep(sc.elementStep);
    config.setBeamFirstElement(sc.firstElement);
    config.setBeamLastElement(sc.lastElement);
    config.setBeamAngleStep(sc.angle);
    config.setFocusMode(static_cast<FocalType>(sc.focusMode));
    config.setFocalPosition(sc.focalPosition);
    config.setTransmissionChannel(sc.transmission);
    config.setReceptionChannel(sc.reception);

    // GatePage
    config.setGateAEnable(sc.gateAEnable);
    config.setGateASynchronMode(static_cast<GateSynchron>(sc.gateASync));
    config.setGateAStart(sc.gateAStart);
    config.setGateAEnd(sc.gateAEnd);
    config.setGateAThreshold(sc.gateAThreshold);
    config.setGateAMeasureType(static_cast<measureType>(sc.gateAMeasureType));
    config.setGateBEnable(sc.gateBEnable);
    config.setGateBSynchronMode(static_cast<GateSynchron>(sc.gateBSync));
    config.setGateBStart(sc.gateBStart);
    config.setGateBEnd(sc.gateBEnd);
    config.setGateBThreshold(sc.gateBThreshold);
    config.setGateBMeasureType(static_cast<measureType>(sc.gateBMeasureType));
    config.setGateIEnable(sc.gateIEnable);
    config.setGateIStart(sc.gateIStart);
    config.setGateIEnd(sc.gateIEnd);
    config.setGateIThreshold(sc.gateIThreshold);
    config.setGateIMeasureType(static_cast<measureType>(sc.gateIMeasureType));
    config.setGateISyncSample(sc.gateISyncSample);
    config.setGateCEnable(sc.gateCEnable);
    config.setGateCSynchronMode(static_cast<GateSynchron>(sc.gateCSync));
    config.setGateCStart(sc.gateCStart);
    config.setGateCEnd(sc.gateCEnd);
    config.setGateCThreshold(sc.gateCThreshold);
    config.setGateCMeasureType(static_cast<measureType>(sc.gateCMeasureType));

    // TcgPage
    config.setTcgEnable(sc.tcgEnable);
    for (int i = 0; i < sc.tcgNums; i++) {
        int index = i + 1;
        config.setTcgPointDepthIndex(index, sc.tcgDepths[i]);
        config.setTcgPointGainIndex(index, sc.tcgGains[i]);
    }

    // Restore per-beam TCG data
    for (int b = 0; b < sc.beamTcgCount; b++) {
        auto &beamPts = sc.beamTcgData[b];
        for (int j = 0; j + 1 < beamPts.size(); j += 2) {
            int index = j / 2 + 1;
            config.setTcgPointDepth(index, beamPts[j], b);
            config.setTcgPointGain(index, beamPts[j + 1], b);
        }
    }

    // ScanPage
    App::getInstance()->currentPosition = sc.resolution;
    App::getInstance()->ScanStart = sc.scanStart;
    App::getInstance()->ScanEnd = sc.scanEnd;
    App::getInstance()->xNormal = sc.encoderDir;
    App::getInstance()->resolutionEcoder1 = sc.xResolution;
    App::getInstance()->position = sc.xPosition;
    App::getInstance()->SoundStart = sc.soundStart;
    App::getInstance()->SoundEnd = sc.soundEnd;
    App::getInstance()->C1 = static_cast<App::Cscan_Data>(sc.c1Data);
    App::getInstance()->C2 = static_cast<App::Cscan_Data>(sc.c2Data);

    // 编码器/时间模式参数
    App::getInstance()->EncoderIndex = sc.encoderIndex;
    App::getInstance()->EncoderSwapped = (sc.encoderSwapped != 0);
    App::getInstance()->resolutionEcoder2 = sc.yResolution;
    App::getInstance()->yNormal = sc.yNormal;
    App::getInstance()->scanResolution = sc.scanResolution;
    App::getInstance()->End = sc.endPosition;
    App::getInstance()->timeResolution = sc.timeResolution;
    App::getInstance()->Scanlength = sc.scanLength;

    // 通知配置已更新
    emit App::getInstance()->refresh_Allpara();

    // 通知各参数变更，让ViewWidget刷新缓存参数
    emit App::getInstance()->signal_RangeChanged();
    emit App::getInstance()->signal_RectifierChanged();
    emit App::getInstance()->signal_AmplitudeChanged();
    emit App::getInstance()->signal_GateView_Refresh();

    // 通知编码器/时间模式切换（让ViewWidget的Encodemode正确设置）
    emit App::getInstance()->signal_Reset_CScan(sc.encoderTimeMode);
}
