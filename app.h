#ifndef APP_H
#define APP_H

#include "qdatetime.h"
#include "qdebug.h"
#include "qthread.h"
#include <QMutex>
#include <QMap>
#include <QObject>
#include <QRect>

class App : public QObject
{
    Q_OBJECT
public:
    // 改为静态局部变量方式
    static App *getInstance()
    {
        static App instance; // 静态局部变量
        return &instance;
    }

    // 删除静态成员变量声明
    // static App *instance;  // 删除这行

    // 设置主窗体
    void setMainWindow(QWidget *);
    QRect getMainWindowRect();

    bool Replay = false; //

    /************/ // 库没写帧数接口，加一个数值表示当前帧步进;

    double currentPosition = 1.0; // 当前帧步进

    double timeResolution = 1; // Hz
    int Scanlength = 60;       // 时间长度;

    // 扫查轴x分辨率
    double resolutionEcoder1 = 12.0; // step/mm
    // 当前位置
    double position = 0.0;
    // 步进轴y分辨率
    double resolutionEcoder2 = 12.0; // step/mm
    bool EncoderSwapped = false;     // false x_y true y_x

    int xNormal = 0; // 0 正向 1 反转
    int yNormal = 0;

    int EncoderIndex = 0; // 0 单轴；1 双轴
    int EncoderTimeMode = 0; // 0 编码器模式；1 时间模式
    /************/        // 记录当前TCG点的个数;
    std::vector<int> TCG_Nums;

    // 步进轴扫查分辨率和终点位置mm
    double scanResolution = 28.8;
    double End = 28.8;

    /************/ // 当前的波束值;
    int CurrentBeam = 1;

    /************/ // 记录当前的测量值;
    double SA = 0;
    double SB = 0;
    double SI = 0;
    double SC = 0;

    /************/ // 声程起点和终点;
    double SoundStart = 0;
    double SoundEnd = 100;
    /************/ // 扫查起点和终点;
    double ScanStart = 0;
    double ScanEnd = 100;
    /************/ // C扫数据源;

    enum Cscan_Data { Gate_A, Gate_B, Gate_C, Gate_I, Gate_SA, Gate_SB, Gate_SC, Gate_SI };
    Cscan_Data C1 = Cscan_Data::Gate_A;
    Cscan_Data C2 = Cscan_Data::Gate_SA;

    // 录制缓冲区：保存时写入文件，上限10000帧防内存堆积
    QMap<quint32, QByteArray> recordBuffer;
    QMutex recordBufferMutex;

signals:
    void refresh_Allpara();
    void signal_showhideGatestatus();
    void signal_probeChange();
    void signal_Reset_CScan(int value);
    void signal_GateView_Refresh();
    void signal_RangeChanged();
    void signal_RectifierChanged();
    void signal_AmplitudeChanged();
    void signal_Connected();
    void signal_xPosition(double position);
    void signal_beamChange();
    void signal_frameChanged(int x);
    void signal_paletteChanged();
    void signal_soundChanged();
    void signal_tcgChanged();

private:
    // 私有构造函数和析构函数
    explicit App(QObject *parent = nullptr);
    ~App() = default;

    // 禁用拷贝构造和赋值操作
    App(const App &) = delete;
    App &operator=(const App &) = delete;

    QWidget *mainWindow = nullptr;
};

#endif // APP_H
