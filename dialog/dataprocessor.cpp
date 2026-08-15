#include "dataprocessor.h"



#include "app.h"
#include "client.h"
#include "measurewidget.h"
#include "viewwidget.h"
DataProcessor::DataProcessor(QObject *parent)
    : QObject(parent)
    , m_config(Client::getInstance())
{
    m_isRunning.store(true);
    m_processing.store(false);
    m_queueEmpty.store(true);
}

DataProcessor::~DataProcessor()
{
    disconnect();
    stop();
}

void DataProcessor::setViews(ViewWidget *v1, ViewWidget *v2, ViewWidget *v3, ViewWidget *v4,
                              MeasureWidget *measure)
{
    m_view1 = v1;
    m_view2 = v2;
    m_view3 = v3;
    m_view4 = v4;
    m_measureWidget = measure;
}

void DataProcessor::setMaxQueueSize(int size)
{
    m_maxQueueSize = qMax(10, size);
}

void DataProcessor::setAllViewsActive(bool active)
{
    m_viewActive = active;
}

void DataProcessor::enqueueData(const QByteArray &data, int device_id)
{
    if (!m_isRunning.load()) {
        return;
    }

    if (m_config.getCurrentServerId() != device_id) {
        return;
    }

    QMutexLocker locker(&m_mutex);

    // 检查队列是否已满
    if (m_queue.size() >= m_maxQueueSize) {
        // 队列已满，丢弃最旧的数据
        m_queue.dequeue();
        m_stats.framesDropped++;
        m_consecutiveDrops++;

        // 如果连续丢弃太多帧，发出警告
        if (m_consecutiveDrops >= 10) {
            qint64 now = QDateTime::currentMSecsSinceEpoch();
            if (now - m_lastWarningTime > 3000) { // 每3秒最多警告一次
                emit performanceWarning(QString("High data loss: %1 consecutive frames dropped")
                                            .arg(m_consecutiveDrops));
                m_lastWarningTime = now;
            }
        }
    } else {
        m_consecutiveDrops = 0;
    }

    // 添加新数据
    FrameData frame;
    frame.data = data;
    frame.timestamp = QDateTime::currentMSecsSinceEpoch();
    frame.enqueueTime = QDateTime::currentMSecsSinceEpoch();
    m_queue.enqueue(frame);
    m_queueEmpty.store(false);

    m_stats.currentQueueSize = m_queue.size();

    // 如果不在处理中，直接处理数据（不通过 invokeMethod，减少一次事件投递延迟）
    if (!m_processing.load()) {
        locker.unlock();
        processDataImmediately();
    }
}

void DataProcessor::processDataImmediately()
{
    if (!m_isRunning.load() || m_processing.load() || m_queueEmpty.load()) {
        return;
    }

    m_processing.store(true);

    // 处理所有队列中的数据
    while (m_isRunning.load() && !m_queueEmpty.load()) {
        processNextFrame();

        // 检查队列是否还有数据
        QMutexLocker locker(&m_mutex);
        m_queueEmpty.store(m_queue.isEmpty());
    }

    m_processing.store(false);
}

void DataProcessor::processNextFrame()
{
    FrameData frame;
    bool hasData = false;

    // 从队列获取数据
    {
        QMutexLocker locker(&m_mutex);
        if (!m_queue.isEmpty()) {
            frame = m_queue.dequeue();
            hasData = true;
            m_stats.currentQueueSize = m_queue.size();
        }
    }

    if (!hasData) {
        m_queueEmpty.store(true);
        return;
    }

    QElapsedTimer timer;
    timer.start();

    // 每次获取最新的beamCount，避免配置变更后不匹配
    int currentBeamCount = m_config.getBeamCounts();

    // ---- 数据录制（从 MainWindow 移入，在处理器线程执行）----
    if (!App::getInstance()->Replay && m_savingEnabled) {
        datapacketTail tail;
        const QByteArray &data = frame.data;
        if (data.size() >= static_cast<int>(sizeof(datapacketTail))) {
            memcpy(&tail, data.data() + data.size() - sizeof(tail), sizeof(tail));
        quint32 key;
        if (App::getInstance()->EncoderTimeMode == 1) {
            key = m_recFrameSeq.fetch_add(1, std::memory_order_relaxed);
        } else if (App::getInstance()->EncoderIndex == 0) {
            if (m_recFirstFrame) {
                m_recFirstEncoderA = tail.encoderA_pos;
                m_recFirstEncoderB = tail.encoderB_pos;
                m_recFirstFrame = false;
            }
            int32_t encoderPos = tail.encoderA_pos;
            int32_t firstEnc = m_recFirstEncoderA;
            double resolution = App::getInstance()->resolutionEcoder1;
            if (App::getInstance()->EncoderSwapped) {
                encoderPos = tail.encoderB_pos;
                firstEnc = m_recFirstEncoderB;
                resolution = App::getInstance()->resolutionEcoder2;
            }
            double dist = (encoderPos - firstEnc)
                * App::getInstance()->currentPosition / resolution;
            if (App::getInstance()->xNormal == 1)
                dist = -dist;
            if (dist < 0)
                dist = 0;
            double imgWidth =
                App::getInstance()->ScanEnd - App::getInstance()->ScanStart;
            int numCols =
                qMax(1, (int)(imgWidth / App::getInstance()->currentPosition));
            key = (quint32)qBound(0, (int)(dist / imgWidth * numCols), numCols - 1);
        } else {
            if (m_recFirstFrame) {
                m_recFirstEncoderA = tail.encoderA_pos;
                m_recFirstEncoderB = tail.encoderB_pos;
                m_recFirstFrame = false;
            }
            int32_t encoderPos = tail.encoderA_pos;
            int32_t firstEnc = m_recFirstEncoderA;
            double resolution = App::getInstance()->resolutionEcoder1;
            if (App::getInstance()->EncoderSwapped) {
                encoderPos = tail.encoderB_pos;
                firstEnc = m_recFirstEncoderB;
                resolution = App::getInstance()->resolutionEcoder2;
            }
            double dist = (encoderPos - firstEnc)
                * App::getInstance()->currentPosition / resolution;
            if (App::getInstance()->xNormal == 1)
                dist = -dist;
            if (dist < 0)
                dist = 0;
            double imgWidth =
                App::getInstance()->ScanEnd - App::getInstance()->ScanStart;
            int numCols =
                qMax(1, (int)(imgWidth / App::getInstance()->currentPosition));
            key = (quint32)qBound(0, (int)(dist / imgWidth * numCols), numCols - 1);
        }
        {
            QMutexLocker locker(&App::getInstance()->recordBufferMutex);
            App::getInstance()->recordBuffer.insert(key, data);
        }
        }
    }

    // ---- 从处理器线程直接分发数据到视图（DirectConnection = 当前线程执行）----
    if (m_viewActive) {
        if (m_view1)
            QMetaObject::invokeMethod(m_view1, "slot_Recive_date", Qt::DirectConnection,
                                      Q_ARG(int, currentBeamCount), Q_ARG(QByteArray, frame.data),
                                      Q_ARG(bool, false), Q_ARG(int, 0));
        if (m_view2)
            QMetaObject::invokeMethod(m_view2, "slot_Recive_date", Qt::DirectConnection,
                                      Q_ARG(int, currentBeamCount), Q_ARG(QByteArray, frame.data),
                                      Q_ARG(bool, false), Q_ARG(int, 0));
        if (m_view3)
            QMetaObject::invokeMethod(m_view3, "slot_Recive_date", Qt::DirectConnection,
                                      Q_ARG(int, currentBeamCount), Q_ARG(QByteArray, frame.data),
                                      Q_ARG(bool, false), Q_ARG(int, 0));
        if (m_view4)
            QMetaObject::invokeMethod(m_view4, "slot_Recive_date", Qt::DirectConnection,
                                      Q_ARG(int, currentBeamCount), Q_ARG(QByteArray, frame.data),
                                      Q_ARG(bool, false), Q_ARG(int, 0));
        if (m_measureWidget)
            QMetaObject::invokeMethod(m_measureWidget, "slot_Recive_date", Qt::DirectConnection,
                                      Q_ARG(QByteArray, frame.data));
    }

    // 更新统计
    updateStatistics(timer.elapsed());
    m_stats.framesProcessed++;
}

void DataProcessor::updateStatistics(qint64 processTime)
{
    m_stats.totalProcessTime += processTime;

    // 每处理100帧更新一次平均时间
    if (m_stats.framesProcessed % 100 == 0) {
        m_stats.averageFrameTime = m_stats.totalProcessTime / 100.0;
        m_stats.totalProcessTime = 0;

        // 检查性能问题
        checkPerformance();
    }
}

void DataProcessor::checkPerformance()
{
    // 如果平均处理时间较长，发出警告
    if (m_stats.averageFrameTime > 60.0) { // 超过16ms（约60FPS）
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - m_lastWarningTime > 5000) { // 每5秒最多警告一次
            QString warning = QString("High processing time: %1ms per frame. ")
                                  .arg(m_stats.averageFrameTime, 0, 'f', 1);

            emit performanceWarning(warning);
            m_lastWarningTime = now;
        }
    }
}

DataProcessor::Statistics DataProcessor::statistics() const
{
    QMutexLocker locker(&m_mutex);
    return m_stats;
}

int DataProcessor::queueSize() const
{
    QMutexLocker locker(&m_mutex);
    return m_queue.size();
}
