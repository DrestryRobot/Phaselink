#ifndef VIEWWIDGET_H
#define VIEWWIDGET_H

#include <QCategoryAxis>
#include <QChartView>
#include <QElapsedTimer>
#include <QFrame>
#include <QGridLayout>
#include <QImage>
#include <QLabel>
#include <QLineSeries>
#include <QMouseEvent>
#include <QMutex>
#include <QQueue>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSlider>
#include <QThread>
#include <QVector>
#include <QWidget>

#include "app.h"
#include "axis_utils.h"

class ViewWorker;
#include "client.h"
#include "dialog/sider.h"
#include "qtimer.h"
#include "ui_viewwidget.h"

// QT_CHARTS_USE_NAMESPACE

///**************************************/数据包末尾时间结构体
// #pragma pack(2)
struct datapacketTail
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
// #pragma pack()
///**************************************/Beam后面的测量值结构体
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

namespace Ui {
class ViewWidget;
}

class ViewWidget : public QWidget
{
    Q_OBJECT

public:
    enum SCAN_TYPE { A_Scan, C1_Scan, C2_Scan, E_Scan };

    enum RenderQuality { Quality_High, Quality_Medium, Quality_Low, Quality_Fast };

    // 设置编码器模式;
    enum EncoderTime { Encoder, Time };

    void updateConfigCache();

    explicit ViewWidget(QWidget *parent = nullptr);
    ~ViewWidget();

    // 视图配置
    void setScanView(SCAN_TYPE type);
    void setColorPalette(const QVector<QRgb> &colors);
    void setRenderQuality(RenderQuality quality);
    void setUpdateInterval(int ms);

    // 数据接口
    Q_INVOKABLE void slot_Recive_date(int beam, const QByteArray &datapacket, bool Replay = false, int No = 0);

    // 闸门控制
    void showGateView();
    void slot_refreshGateStatue();

    // C扫图相关
    void initCsacnView();
    void setDragRange(int min, int max);
    void resetDragBar();
    void dragBarShow(bool flag = true);

    // 性能控制
    void setActive(bool active);
    bool isActive() const { return m_isActive; }
    qreal averageRenderTime() const { return m_avgRenderTime; }
    int framesProcessed() const { return m_framesProcessed; }

    // 视图组件访问
    QLabel *pixLabel() const { return Pix_Label; }
    QChartView *chartView() const { return m_chartView; }
    Sider *m_sider = new Sider();

    void renderNextFrame();
    bool Isstart = true;

    void onViewResized();

    void processFrameData(App::Cscan_Data C_SCAN, int beam, const QByteArray &datapacket,
                          bool Replay, int No, int frameNo, double yPos = 0);
    // 播放拖动条控制
    void updateBarPosition(int x);
    void setPlaybackSliderRange(int maxFrame);
    void setPlaybackSliderValue(int frame);
    void setFrameLabel(int frame);

    int m_dragMaxCol = 0;

    void setChartColors(const QColor &bgColor, const QColor &lineColor); // 颜色样式

    void setAxisColor(const QColor &color);  // 新增：设置坐标轴颜色

    void clearSeries(); // 清空A扫

signals:
    void playbackFrameChanged(int frame);

protected:
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *e) override;
    bool eventFilter(QObject *watched, QEvent *evt) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    // 初始化函数
    void initChart(QWidget *parent);
    void initLabel(QWidget *parent);
    void initCscanWidget(QWidget *parent);
    void initCscan_Widget(QWidget *parent);

    // 视图渲染函数
    void showAScan(const QByteArray &b, int cnt_fl);
    void showEScan(int w, int h, const QByteArray &src);
    // 优化版本渲染函数
    void showEScanOptimized(int w, int h, const QByteArray &src);

    // 闸门相关
    void slot_GateA_refreshGatedata();
    void slot_GateB_refreshGatedata();
    void slot_GateI_refreshGatedata();
    void slot_Rang_startend_ValueChanged();

    // 性能优化函数
    void updateImageCache(int w, int h, const QByteArray &src);
    void updatePerformanceStats(qint64 renderTime);

private:
    Ui::ViewWidget *ui;
    Client &config;

    // 视图组件
    QChart *m_chart = nullptr;
    QChartView *m_chartView = nullptr;
    QLineSeries *m_series = nullptr;
    QCategoryAxis *xAxis = nullptr;
    QCategoryAxis *yAxis = nullptr;
    QLabel *Pix_Label = nullptr;

    // 闸门线条
    QFrame *GateAline = nullptr;
    QFrame *GateBline = nullptr;
    QFrame *GateCline = nullptr;
    QFrame *GateIline = nullptr;

    // C扫图相关
    // QFrame *dragBar = nullptr;
    bool isDragging = false;
    int dragOffset = 0;
    int barWidth = 3;
    int currentX = 0;
    int minRange = 0;
    int maxRange = 0;

    // 数据缓存
    QVector<double> Amp_TOFData;

    // A-Scan 坐标轴缓存
    double m_lastAxisRangeStart = 0.0;                               // 上次 X 轴范围起点
    double m_lastAxisRangeEnd = 0.0;                                 // 上次 X 轴范围终点
    int m_lastPointQuantity = 0;                                     // 上次点数
    RectifierType m_lastRectifierType = RectifierType::Rectifier_RF; // 上次检波模式
    bool m_axisCacheValid = false;                                   // 坐标轴缓存是否有效

    // 图像缓存
    QImage m_cachedEScanImage;
    QSize m_lastLabelSize;
    int m_lastMaxRate = -1;
    bool m_cacheValid = false;

    // 颜色表
    QVector<QRgb> color_Amplitude;

    // 状态变量
    SCAN_TYPE View = A_Scan;
    int id_beam = 1;
    EncoderTime Encodemode = EncoderTime::Encoder;

    // 编码器位置（跨线程访问，由 m_dataMutex 保护）
    mutable QMutex m_dataMutex;
    int32_t EncoderA_pos = 0;
    int32_t EncoderB_pos = 0;
    qint64 StartTime = 0;

    // 扫描位置
    double xDistance = 0;
    double yDistance = 0;
    double y1 = 0;
    int frames = 0;
    int frameNo = 0;

    // 性能优化
    bool m_isActive = true;
    RenderQuality m_renderQuality = Quality_Medium;
    int m_updateInterval = 0; // 0表示立即更新

    // 性能统计
    qreal m_avgRenderTime = 0;
    int m_framesProcessed = 0;
    qint64 m_totalRenderTime = 0;
    qint64 m_lastRenderTime = 0;
    QElapsedTimer m_renderTimer;
    QTimer m_cScanTimer;

    // 数据队列（用于控制更新频率）
    struct RenderData
    {
        QByteArray data;
        int beam;
        bool replay;
        int no;
        int frameNo;
        qint64 timestamp;
    };

    QQueue<RenderData> m_renderQueue;
    QMutex m_queueMutex;
    QTimer m_renderTimerControl;

    // 统计信息
    int m_framesSkipped = 0;
    int m_maxQueueSize = 5; // 最大渲染队列长度

    double range_start = 0.0;
    double range_end = 50.0;
    int pointQuantity = 1024;
    double rePointQuantity = 0.0;
    double maxAmp = 4096.0;
    int groupdata_size = 0;
    RectifierType reType = RectifierType::Rectifier_RF;

    QPixmap m_cachedPixmap;
    QSize m_lastViewSize;

    // 闸门参数缓存（避免拖动时频繁调用config.get）
    bool m_gateAEnable = false;
    bool m_gateBEnable = false;
    bool m_gateCEnable = false;
    bool m_gateIEnable = false;
    double m_gateAStart = 0, m_gateAEnd = 0, m_gateAThreshold = 0;
    double m_gateBStart = 0, m_gateBEnd = 0, m_gateBThreshold = 0;
    double m_gateCStart = 0, m_gateCEnd = 0, m_gateCThreshold = 0;
    double m_gateIStart = 0, m_gateIEnd = 0, m_gateIThreshold = 0;
    GateSynchron m_gateASync = GateSynchron::Pulser;
    GateSynchron m_gateBSync = GateSynchron::Pulser;
    GateSynchron m_gateCSync = GateSynchron::Pulser;

    // 播放竖直指示线（覆盖在C扫图上）
    QFrame *m_playbackLine = nullptr;
    QLabel *m_frameLabel = nullptr;



    // C扫宽高（仅UI计算用，数据在ViewWorker）
    int m_bufferWidth = 0;
    int m_bufferHeight = 0;
    int m_maxFrames = 0;

public:
    void resizePlaybackBuffer(int frameCount);
    int bufferWidth() const { return m_bufferWidth; }

    // A扫回放缓存（beam滑块改变时重新渲染）
    QByteArray m_lastAScanData;
    int m_lastAScanBeam = 0;



    // 统计信息
    std::atomic<int> m_totalFramesReceived { 0 };
    std::atomic<int> m_totalFramesProcessed { 0 };

    // 新增：帧到位图位置的映射缓存
    QHash<int, int> m_frameToXPosCache;

    // 渲染状态
    struct RenderStats
    {
        int queueSize = 0;
        int framesSkipped = 0;
        int bufferUsage = 0;
    } m_stats;

private slots:
    void onAScanReady(QList<QPointF> points);
    void onEScanReady(QImage image);
    void onCScanImageReady(QImage image);

private:
    ViewWorker *m_worker = nullptr;
    QThread *m_workerThread = nullptr;

    void sendConfigToWorker();

};

#endif // VIEWWIDGET_H
