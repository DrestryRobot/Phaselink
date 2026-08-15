#ifndef DATAPROCESSOR_H
#define DATAPROCESSOR_H

#include <QElapsedTimer>
#include <QMutex>
#include <QObject>
#include <QQueue>
#include <atomic>

class Client;
class ViewWidget;
class MeasureWidget;

class DataProcessor : public QObject
{
    Q_OBJECT

public:
    explicit DataProcessor(QObject *parent = nullptr);
    ~DataProcessor();

    // 队列控制
    int queueSize() const;
    int maxQueueSize() const { return m_maxQueueSize; }
    void setMaxQueueSize(int size);

    // 视图控制
    void setAllViewsActive(bool active);

    // 设置视图指针（数据将直接从处理器线程分发到视图）
    void setViews(ViewWidget *v1, ViewWidget *v2, ViewWidget *v3, ViewWidget *v4,
                  MeasureWidget *measure);

    // 启停控制
    void start() { m_isRunning.store(true); }
    void stop()
    {
        m_isRunning.store(false);
        // 清空队列
        QMutexLocker locker(&m_mutex);
        m_queue.clear();
        m_queueEmpty.store(true);
    }
    bool isRunning() const { return m_isRunning.load(); }
    bool isProcessing() const { return m_processing.load(); }

    // 数据录制控制
    void setSavingEnabled(bool enabled) { m_savingEnabled = enabled; }
    bool isSavingEnabled() const { return m_savingEnabled; }
    void resetRecording()
    {
        m_recFirstFrame = true;
        m_recFirstEncoderA = 0;
        m_recFirstEncoderB = 0;
        m_recFrameSeq.store(0, std::memory_order_relaxed);
        m_recFirstTimestamp = 0;
    }

    // 性能统计
    struct Statistics
    {
        int framesProcessed = 0;
        int framesDropped = 0;
        qint64 totalProcessTime = 0;
        qreal averageFrameTime = 0;
        int currentQueueSize = 0;
        qint64 lastUpdateTime = 0;
    };

    Statistics statistics() const;

signals:
    // 状态信号
    void queueOverflow(int droppedFrames);
    void processingStarted();
    void processingStopped();

    // 性能警告
    void performanceWarning(const QString &message);

public slots:
    void enqueueData(const QByteArray &data, int device_id);
    void processDataImmediately(); // 新增：立即处理数据

private:
    void processNextFrame(); // 处理单帧数据
    void updateStatistics(qint64 processTime);
    void checkPerformance();

    struct FrameData
    {
        QByteArray data;
        qint64 timestamp;
        qint64 enqueueTime;
    };

    Client &m_config;
    QQueue<FrameData> m_queue;
    mutable QMutex m_mutex;

    // 配置
    std::atomic<bool> m_isRunning { false };
    std::atomic<bool> m_processing { false };
    std::atomic<bool> m_queueEmpty { true };
    int m_maxQueueSize = 2000;
    bool m_viewActive = true; // View1-4 + Measure

    // 统计
    // 视图指针（从处理器线程直接分发数据）
    ViewWidget *m_view1 = nullptr;
    ViewWidget *m_view2 = nullptr;
    ViewWidget *m_view3 = nullptr;
    ViewWidget *m_view4 = nullptr;
    MeasureWidget *m_measureWidget = nullptr;

    // 数据录制状态（跨线程访问，使用原子类型）
    std::atomic<bool> m_savingEnabled { false };
    std::atomic<int32_t> m_recFirstEncoderA { 0 };
    std::atomic<int32_t> m_recFirstEncoderB { 0 };
    std::atomic<bool> m_recFirstFrame { true };
    std::atomic<quint32> m_recFrameSeq { 0 };
    quint64 m_recFirstTimestamp = 0;

    Statistics m_stats;
    int m_consecutiveDrops = 0;
    qint64 m_lastWarningTime = 0;
    QElapsedTimer m_perfTimer;

};

#endif // DATAPROCESSOR_H
