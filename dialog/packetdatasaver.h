#ifndef PACKETDATASAVER_H
#define PACKETDATASAVER_H

#include <QObject>
#include <QMap>
#include <QByteArray>
#include <QString>
#include <QVector>
#include <client.h>
#include <app.h>
struct TailInform
{
    int16_t frameErr : 1;
    int16_t scannerIO : 3;
    int16_t multiFrame : 12;
    uint16_t frameNo : 12;
    uint16_t encoder_trig_err : 2;
    uint16_t encoder_sync_err : 2;
    int32_t encoderA_pos : 32;
    int32_t encoderB_pos : 32;
    int32_t encoderC_pos : 32;
    int32_t encoderD_pos : 32;
    int32_t encoderE_pos : 32;
    uint64_t timestamp : 64;
};

struct MeasureValue
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

class PacketDataSaver : public QObject
{
    Q_OBJECT

public:
    explicit PacketDataSaver(QObject *parent = nullptr);

    // 设置/获取数据
    void setData(const QMap<quint32, QByteArray> &data);
    const QMap<quint32, QByteArray> &getData() const;

    // 二进制保存/加载（高效格式）
    bool saveToFile(int num, int points, QWidget *parent = nullptr);
    bool saveToFile(const QString &filename, int num, int points);
    bool loadFromFile(QWidget *parent = nullptr);
    bool loadFromFile(const QString &filename);

    int loadedEncoderTimeMode() const { return m_scanConfig.encoderTimeMode; }

signals:
    void progressChanged(int progress);
    void operationCompleted(bool success, const QString &message);

private:
    void applyConfigToClient();

    Client &config;
    QMap<quint32, QByteArray> m_packetMap;

    // 保存/加载时缓存的配置参数
    struct ScanConfig {
        int groupId;
        // Epage
        double gain;
        double rangeStart, rangeEnd;
        double pitch;
        double frequency;
        double longitudinalVelocity;
        double wedgeVelocity;
        int wedgeEnable;
        double wedgeX, wedgeZ;
        double frameRate;
        int voltage, pulseWidth;
        int voltagePolarity;
        double filterHigh, filterLow;
        int rectifierMode;
        int maxAmplitude;
        int points;
        double zeroPosition;
        // Phasepage
        int scanType, waveType, beamMode;
        int aperture, elementStep, firstElement, lastElement;
        double angle;
        int focusMode;
        double focalPosition;
        int transmission, reception;
        // GatePage
        int gateAEnable, gateASync;
        double gateAStart, gateAEnd, gateAThreshold;
        int gateAMeasureType;
        int gateBEnable, gateBSync;
        double gateBStart, gateBEnd, gateBThreshold;
        int gateBMeasureType;
        int gateIEnable;
        double gateIStart, gateIEnd, gateIThreshold;
        int gateIMeasureType, gateISyncSample;
        int gateCEnable, gateCSync;
        double gateCStart, gateCEnd, gateCThreshold;
        int gateCMeasureType;
        // TcgPage
        int tcgEnable;
        int tcgNums;
        QVector<double> tcgDepths;
        QVector<double> tcgGains;
        int beamTcgCount;                      // number of beams with TCG data
        QVector<QVector<double>> beamTcgData;  // per-beam TCG vectors
        // ScanPage
        double resolution;
        double scanStart, scanEnd;
        int encoderDir;
        double xResolution;
        double xPosition;
        double soundStart, soundEnd;
        int c1Data, c2Data;
        // 编码器/时间模式参数（回放必需）
        int encoderIndex;         // 0=单轴 1=双轴
        int encoderSwapped;       // 0=x_y 1=y_x
        double yResolution;       // 步进轴y分辨率
        int yNormal;              // y轴反转
        double scanResolution;    // 步进轴扫查分辨率
        double endPosition;       // 步进轴终点
        double timeResolution;    // 时间分辨率
        int scanLength;           // 时间长度
        int encoderTimeMode;      // 0=编码器 1=时间
        // Data info
        int beamCount;
        int pointCount;
    } m_scanConfig;
};

#endif // PACKETDATASAVER_H
