#include "viewwidget.h"

#include <QCategoryAxis>
#include <QComboBox>
#include <QCoreApplication>
#include <QDebug>
#include <QMetaObject>
#include <QMutexLocker>
#include <QPainter>
#include <QScrollArea>
#include <QSlider>
#include <QThread>
#include <QTimer>
#include <QTransform>
#include <QVector>

#include <QMetaType>

#include "dialog/viewworker.h"
#include "qdatetime.h"
#include "qgraphicslayout.h"
#include "ui_viewwidget.h"

ViewWidget::ViewWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ViewWidget)
    , config(Client::getInstance())
    //, dragBar(nullptr)
    , isDragging(false)
    , m_renderTimerControl(this)
{
    ui->setupUi(this);

    static bool reg = false;
    if (!reg) {
        qRegisterMetaType<QVector<QRgb>>("QVector<QRgb>");
        qRegisterMetaType<QVector<int>>("QVector<int>");
        reg = true;
    }

    m_worker = new ViewWorker();
    m_workerThread = new QThread(this);
    m_worker->moveToThread(m_workerThread);
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &ViewWorker::aScanReady, this, &ViewWidget::onAScanReady);
    connect(m_worker, &ViewWorker::eScanReady, this, &ViewWidget::onEScanReady);
    connect(m_worker, &ViewWorker::cScanImageReady, this, &ViewWidget::onCScanImageReady);
    m_workerThread->start();

    connect(App::getInstance(), &App::signal_Reset_CScan, this, [=](int index) {
        Encodemode = static_cast<EncoderTime>(index);

        QTimer::singleShot(100, this, [=]() { Isstart = true;  initCsacnView();});
    });

    // 连接渲染控制定时器
    connect(&m_renderTimerControl, &QTimer::timeout, this, &ViewWidget::renderNextFrame);

    if (App::getInstance()) {
        connect(App::getInstance(), &App::signal_GateView_Refresh, this, [this]() {
            updateConfigCache();
            showGateView();
        });
        connect(App::getInstance(), &App::signal_RangeChanged, this, [this]() {
            range_start = static_cast<double>(config.getRangeStart());
            range_end = static_cast<double>(config.getRangeEnd());
            pointQuantity = config.getPointQuantity();
            rePointQuantity = 1.0 / pointQuantity;
            // 重新计算 groupdata_size
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
            m_axisCacheValid = false;
            sendConfigToWorker();
            QMetaObject::invokeMethod(m_worker, "resetCScanFrameCache", Qt::QueuedConnection);
            if (View == A_Scan && !m_lastAScanData.isEmpty()) {
                showAScan(m_lastAScanData, m_lastAScanBeam);
            }
        });
        connect(App::getInstance(), &App::signal_RectifierChanged, this, [this]() {
            reType = config.getRectifierMode();
            m_axisCacheValid = false;
            if (View == A_Scan && !m_lastAScanData.isEmpty()) {
                showAScan(m_lastAScanData, m_lastAScanBeam);
            }
        });
        connect(App::getInstance(), &App::signal_AmplitudeChanged, this, [this]() {
            maxAmp = static_cast<double>(config.getMaxAmplitude());
            m_axisCacheValid = false;
            sendConfigToWorker();

        });
        connect(App::getInstance(), &App::signal_soundChanged, this, [this]() {
            QMetaObject::invokeMethod(m_worker, "updateSoundConfig", Qt::QueuedConnection,
                                      Q_ARG(double, App::getInstance()->SoundStart),
                                      Q_ARG(double, App::getInstance()->SoundEnd));
        });
    }

    setAttribute(Qt::WA_StaticContents);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

ViewWidget::~ViewWidget()
{
    // 断开所有连接
    disconnect();

    // 停止所有定时器
    m_renderTimerControl.stop();

    // 清空队列
    m_renderQueue.clear();

    // 停止工作线程
    if (m_workerThread) {
        m_workerThread->requestInterruption();
        m_workerThread->quit();
        if (!m_workerThread->wait(3000)) {
            m_workerThread->terminate();
            m_workerThread->wait();
        }
    }

    // 只删除没有父对象的 Qt 对象（有父对象的会在父对象销毁时自动删除）
    if (m_chart && !m_chart->parent()) {
        delete m_chart;
    }

    delete ui;
}

void ViewWidget::updateConfigCache()
{
    range_start = static_cast<double>(config.getRangeStart());
    range_end = static_cast<double>(config.getRangeEnd());
    pointQuantity = config.getPointQuantity();

    rePointQuantity = 1.0 / pointQuantity;
    maxAmp = static_cast<double>(config.getMaxAmplitude());
    reType = config.getRectifierMode();
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

    // 缓存闸门参数（避免showGateView频繁调用config.get）
    m_gateAEnable = config.getGateAEnable();
    m_gateBEnable = config.getGateBEnable();
    m_gateIEnable = config.getGateIEnable();
    m_gateAStart = config.getGateAStart();

    m_gateAEnd = config.getGateAEnd();
    m_gateAThreshold = config.getGateAThreshold();
    m_gateASync = config.getGateASynchronMode();
    m_gateBStart = config.getGateBStart();
    m_gateBEnd = config.getGateBEnd();
    m_gateBThreshold = config.getGateBThreshold();
    m_gateBSync = config.getGateBSynchronMode();
    m_gateIStart = config.getGateIStart();
    m_gateIEnd = config.getGateIEnd();
    m_gateIThreshold = config.getGateIThreshold();
    m_gateCEnable = config.getGateCEnable();
    m_gateCStart = config.getGateCStart();
    m_gateCEnd = config.getGateCEnd();
    m_gateCThreshold = config.getGateCThreshold();
    m_gateCSync = config.getGateCSynchronMode();

    // 参数变化，坐标轴缓存失效
    m_axisCacheValid = false;
}

void ViewWidget::initChart(QWidget *parent)
{
    m_chart = new QChart();
    m_chart->legend()->hide();
    m_chart->layout()->setContentsMargins(0, 0, 0, 0);

    m_chartView = new QChartView(m_chart, parent);
    m_series = new QLineSeries;

    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setRubberBand(QChartView::NoRubberBand);
    // 波形大幅跳变（如探头进出板子）时，Qt Charts 增量重绘可能残留旧轨迹，
    // 强制全量重绘并给绘图区不透明背景，确保每帧都把旧波形彻底擦掉。
    m_chartView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    m_chart->setPlotAreaBackgroundVisible(true);
    m_chart->setPlotAreaBackgroundBrush(QBrush(Qt::black));

    auto beamPoints = pointQuantity;
    auto xtick_count = 5;
    xAxis = new QCategoryAxis();
    xAxis->setRange(0, beamPoints);
    xAxis->setLabelsPosition(QCategoryAxis::AxisLabelsPosition::AxisLabelsPositionOnValue);
    auto a = AxisInfos::generateAxisInfo(range_start, beamPoints, xtick_count, range_end, "");
    for (int i = 0; i < xtick_count; i++) {
        xAxis->append(a[i]->label, a[i]->pos);
    }

    auto amp = 100;
    int ytick_count = 11;
    yAxis = new QCategoryAxis();
    yAxis->setRange(0, amp);
    yAxis->setLabelsPosition(QCategoryAxis::AxisLabelsPosition::AxisLabelsPositionOnValue);
    yAxis->setTickCount(ytick_count);
    auto a2 = AxisInfos::generateAxisInfo(0, amp, ytick_count, amp, "%");
    for (int i = 0; i < ytick_count; i++) {
        yAxis->append(a2.at(i)->label, a2.at(i)->pos);
    }

    m_chart->addAxis(xAxis, Qt::AlignBottom);
    m_chart->addAxis(yAxis, Qt::AlignLeft);

    m_chart->addSeries(m_series);
    m_series->attachAxis(xAxis);
    m_series->attachAxis(yAxis);

    m_sider->silder()->setOrientation(Qt::Horizontal);
    m_sider->silder()->setMinimumHeight(40);

    QGridLayout *parentLayout = new QGridLayout();
    parentLayout->addWidget(m_chartView);
    parentLayout->addWidget(m_sider);

    parentLayout->setContentsMargins(0, 0, 0, 0);
    parentLayout->setSpacing(0);
    parent->setLayout(parentLayout);
    connect(m_sider->silder(), &QSlider::valueChanged, this, [=](int value) {
        id_beam = value;
        App::getInstance()->CurrentBeam = value;
        emit App::getInstance()->signal_beamChange();
        // 有闸门同步时需要刷新闸门位置
        if (m_gateASync == GateSynchron::GateI || m_gateBSync == GateSynchron::GateI
            || m_gateBSync == GateSynchron::GateA || m_gateCSync == GateSynchron::GateI
            || m_gateCSync == GateSynchron::GateA || m_gateCSync == GateSynchron::GateB) {
            showGateView();
        }
        // 回放时用缓存数据重新渲染A扫（用户切换beam）
        if (!m_lastAScanData.isEmpty()) {
            showAScan(m_lastAScanData, m_lastAScanBeam);
        }
    });

    GateAline = new QFrame(m_chartView);
    GateAline->setStyleSheet("QWidget{background-color:#e00000;max-height: 1px;}");
    GateAline->setFrameShape(QFrame::HLine);
    GateAline->setFrameShadow(QFrame::Sunken);
    GateAline->raise();
    GateAline->hide();
    GateAline->installEventFilter(this);

    GateBline = new QFrame(m_chartView);
    GateBline->setStyleSheet("QWidget{background-color:#00ff00;max-height: 1px}");
    GateBline->setFrameShape(QFrame::HLine);
    GateBline->setFrameShadow(QFrame::Sunken);
    GateBline->hide();
    GateBline->raise();
    GateBline->installEventFilter(this);

    GateIline = new QFrame(m_chartView);
    GateIline->setStyleSheet("QWidget{background-color:#ffff7f;max-height: 1px}");
    GateIline->setFrameShape(QFrame::HLine);
    GateIline->setFrameShadow(QFrame::Sunken);
    GateIline->hide();
    GateIline->raise();
    GateIline->installEventFilter(this);

    GateCline = new QFrame(m_chartView);
    GateCline->setStyleSheet("QWidget{background-color:#00bfff;max-height: 1px}");
    GateCline->setFrameShape(QFrame::HLine);
    GateCline->setFrameShadow(QFrame::Sunken);
    GateCline->hide();
    GateCline->raise();
    GateCline->installEventFilter(this);

    if (App::getInstance()) {
        connect(App::getInstance(), &App::signal_showhideGatestatus, this, [this]() {
            updateConfigCache();
            slot_refreshGateStatue();
        });
    }
}

void ViewWidget::initLabel(QWidget *parent)
{
    Pix_Label = new QLabel(parent);
    QGridLayout *parentLayout = new QGridLayout();
    parentLayout->addWidget(Pix_Label);
    parentLayout->setContentsMargins(0, 0, 0, 0);
    parentLayout->setSpacing(0);
    parent->setLayout(parentLayout);
}

void ViewWidget::initCscanWidget(QWidget *parent)
{
    Pix_Label = new QLabel();
    Pix_Label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setDragRange(App::getInstance()->ScanStart, App::getInstance()->ScanEnd);

    // 竖直方向粗线，深蓝色，覆盖在C扫图上
    m_playbackLine = new QFrame(Pix_Label);
    m_playbackLine->setFrameShape(QFrame::VLine);
    m_playbackLine->setStyleSheet("QFrame{color: #5c9cf5; background-color: #5c9cf5;}");
    m_playbackLine->setLineWidth(2);
    m_playbackLine->setFixedWidth(2);
    m_playbackLine->hide();

    m_frameLabel = new QLabel(Pix_Label);
    m_frameLabel->setStyleSheet("QLabel{color:#00ff00; font-size:10px; background:rgba(0,0,0,120); padding:2px;}");
    m_frameLabel->setAlignment(Qt::AlignRight | Qt::AlignTop);
    m_frameLabel->raise();

    m_cScanTimer.setInterval(50);
    connect(&m_cScanTimer, &QTimer::timeout, this, [this]() {
        QMetaObject::invokeMethod(m_worker, "renderCScan", Qt::QueuedConnection);
    });
    m_cScanTimer.start();

    QGridLayout *parentLayout = new QGridLayout();
    parentLayout->setContentsMargins(0, 0, 0, 0);
    parentLayout->setSpacing(0);
    parentLayout->addWidget(Pix_Label, 0, 0, 1, 1);
    parent->setLayout(parentLayout);
}

void ViewWidget::initCscan_Widget(QWidget *parent)
{
    Pix_Label = new QLabel(parent);
    QGridLayout *parentLayout = new QGridLayout();
    parentLayout->addWidget(Pix_Label);
    parentLayout->setContentsMargins(0, 0, 0, 0);
    parentLayout->setSpacing(0);
    parent->setLayout(parentLayout);
}

void ViewWidget::initCsacnView()
{
    Isstart = true;

    // 清空旧数据
    m_frameToXPosCache.clear();

    if (View == C1_Scan || View == C2_Scan) {
        // 计算图像尺寸
        double imgWidth;
        if (Encodemode == EncoderTime::Encoder) {
            imgWidth = App::getInstance()->ScanEnd - App::getInstance()->ScanStart;
            double curPos = App::getInstance()->currentPosition;
            frames = (curPos > 0) ? static_cast<int>(imgWidth / curPos) : 1;
        } else {
            imgWidth = App::getInstance()->Scanlength;
            double timeRes = App::getInstance()->timeResolution;
            frames = (timeRes > 0) ? static_cast<int>(imgWidth / timeRes) : 1;
        }
        if (frames < 1)
            frames = 1;

        double scanRes = App::getInstance()->scanResolution;
        int pre = (scanRes > 0) ? static_cast<int>(App::getInstance()->End / scanRes) : 1;
        if (pre < 1)
            pre = 1;
        int imgHeight = (App::getInstance()->EncoderIndex == 1) ? pre * config.getBeamCounts()
                                                                : config.getBeamCounts();

        // 限制最大缓冲区避免OOM
        if (frames > 100000)
            frames = 100000;
        if (imgHeight > 100000)
            imgHeight = 100000;

        m_bufferWidth = frames;
        m_bufferHeight = imgHeight;

        QMetaObject::invokeMethod(m_worker, "resizeCScanBuffers", Qt::QueuedConnection,
                                  Q_ARG(int, frames), Q_ARG(int, imgHeight),
                                  Q_ARG(int, config.getBeamCounts()));
    }

    m_maxFrames = frames;
    m_totalFramesReceived = 0;
    m_totalFramesProcessed = 0;

    setPlaybackSliderRange(m_maxFrames - 1);
}

void ViewWidget::setScanView(SCAN_TYPE type)
{
    View = type;
    if (type == A_Scan) {
        initChart(ui->main_widget);
    } else if (type == C1_Scan || type == C2_Scan) {
        initCscanWidget(ui->main_widget);
        bool isAmp = (type == C1_Scan);
        QMetaObject::invokeMethod(m_worker, "setCScanViewIsAmp", Qt::QueuedConnection,
                                  Q_ARG(bool, isAmp));
    } else if (type == E_Scan) {
        initLabel(ui->main_widget);
    }

    if (type == A_Scan) {
        setRenderQuality(Quality_Low);
    } else if (type == E_Scan) {
        setRenderQuality(Quality_Low);
    }
}

void ViewWidget::changeEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
    QWidget::changeEvent(event);
}

void ViewWidget::showEvent(QShowEvent *event)
{
    // showGateView();

    QWidget::showEvent(event);
}

bool ViewWidget::eventFilter(QObject *watched, QEvent *evt)
{
    // // 闸门拖动事件处理
    // static QPoint mousePoint;
    // static bool mousePressed = false;
    // QMouseEvent *event = static_cast<QMouseEvent *>(evt);

    // if (watched == GateAline || watched == GateBline || watched == GateIline) {
    //     if (event->type() == QEvent::MouseButtonPress) {
    //         if (event->button() == Qt::LeftButton) {
    //             mousePressed = true;
    //             mousePoint = event->globalPos() - static_cast<QFrame *>(watched)->pos();
    //             return true;
    //         }
    //     } else if (event->type() == QEvent::MouseButtonRelease) {
    //         mousePressed = false;

    //         if (watched == GateAline) {
    //             slot_GateA_refreshGatedata();
    //         } else if (watched == GateBline) {
    //             slot_GateB_refreshGatedata();
    //         } else if (watched == GateIline) {
    //             slot_GateI_refreshGatedata();
    //         }
    //         return true;
    //     } else if (event->type() == QEvent::MouseMove) {
    //         if (mousePressed && (event->buttons() & Qt::LeftButton)) {
    //             static_cast<QFrame *>(watched)->move(event->globalPos() - mousePoint);
    //             return true;
    //         }
    //     }
    // }
    return QWidget::eventFilter(watched, evt);
}

void ViewWidget::resizeEvent(QResizeEvent *e)
{
    showGateView();
    QWidget::resizeEvent(e);
}

void ViewWidget::slot_Recive_date(int beam, const QByteArray &datapacket, bool Replay, int No)
{
    // 探针：测量 UI 线程上每帧收包处理耗时（RAII，覆盖提前返回路径）
    struct RecvProbe {
        QElapsedTimer t;
        RecvProbe() { t.start(); }
        ~RecvProbe() {
            static qint64 ms = 0;
            static int n = 0;
            ms += t.nsecsElapsed() / 1000;
            n++;
            if (ms >= 500000) {
                qDebug() << "[RecvTiming] slot_Recive_date平均" << (ms / (double)n)
                         << "µs, n=" << n;
                ms = 0; n = 0;
            }
        }
    } recvProbe;

    if (!m_isActive || datapacket.isEmpty()) {
        return;
    }

    // 调试：A 扫视图每秒统计收到的帧数
    if (View == A_Scan) {
        static int aFrames = 0;
        static QElapsedTimer aTimer;
        if (!aTimer.isValid())
            aTimer.start();
        aFrames++;
        if (aTimer.elapsed() >= 1000) {
            qDebug() << "[AScan] View_1 收到帧:" << aFrames
                     << " id_beam:" << id_beam
                     << " pointQuantity:" << pointQuantity
                     << " maxAmp:" << maxAmp
                     << " 包大小:" << datapacket.size();
            aFrames = 0;
            aTimer.restart();
        }
    }

    const int minSize = static_cast<int>(sizeof(datapacketTail));
    if (datapacket.size() < minSize) {
        qWarning() << "ViewWidget: datapacket too small, size=" << datapacket.size();
        return;
    }

    datapacketTail TailInform;
    memcpy(&TailInform, datapacket.data() + datapacket.size() - sizeof(TailInform),
           sizeof(TailInform));

    // ---- 计算编码器位置（跨线程保护）----
    double local_xDistance = 0;
    double local_yDistance = 0;
    int local_frameNo = 0;
    double local_frames = 0;

    {
        QMutexLocker locker(&m_dataMutex);

        if (App::getInstance()->EncoderIndex == 0) {           // 单轴 en:Single-axis
            if (App::getInstance()->EncoderSwapped == false) { // X轴 en:X-axis
                if (Encodemode == EncoderTime::Encoder) {
                    if (Isstart) {
                        EncoderA_pos = TailInform.encoderA_pos;
                    }

                    xDistance = (TailInform.encoderA_pos - EncoderA_pos)
                        / App::getInstance()->resolutionEcoder1 * App::getInstance()->currentPosition;

                    if (App::getInstance()->xNormal == 1) {
                        xDistance = -xDistance;
                    }
                    if (xDistance < 0) {
                        xDistance = 0;
                    }
                    emit App::getInstance()->signal_xPosition(xDistance);
                    Isstart = false;
                }
            } else if (App::getInstance()->EncoderSwapped == true) {
                if (Encodemode == EncoderTime::Encoder) {
                    if (Isstart) {
                        EncoderB_pos = TailInform.encoderB_pos;
                    }
                    Isstart = false;
                    if (xDistance < 0) {
                        xDistance = 0;
                    } else {
                        xDistance = (TailInform.encoderB_pos - EncoderB_pos)
                            / App::getInstance()->resolutionEcoder2
                            * App::getInstance()->currentPosition;
                        if (App::getInstance()->yNormal == 1) {
                            xDistance = -xDistance;
                        }

                        emit App::getInstance()->signal_xPosition(xDistance);
                    }
                }
            }
            if (Encodemode == EncoderTime::Time) { // 时间 en:Time
                if (Isstart) {
                    StartTime = TailInform.timestamp;
                }
                Isstart = false;
                if (xDistance > App::getInstance()->Scanlength) {
                    xDistance = App::getInstance()->Scanlength;
                } else {
                    xDistance = (TailInform.timestamp - StartTime) / (1000000.0);
                }
            }

            App::getInstance()->position = xDistance;
        } else if (App::getInstance()->EncoderIndex == 1) {    // 双轴 en:Double axis
            if (App::getInstance()->EncoderSwapped == false) { // x_y
                if (Encodemode == EncoderTime::Encoder) {
                    if (Isstart) {
                        EncoderA_pos = TailInform.encoderA_pos;
                        EncoderB_pos = TailInform.encoderB_pos;
                    }
                    Isstart = false;

                    if (xDistance < 0) {
                        xDistance = 0;
                    } else {
                        xDistance = (TailInform.encoderA_pos - EncoderA_pos)
                            / App::getInstance()->resolutionEcoder1
                            * App::getInstance()->currentPosition;
                        if (App::getInstance()->xNormal == 1) {
                            xDistance = -xDistance;
                        }
                    }

                    yDistance = (TailInform.encoderB_pos - EncoderB_pos)
                        / App::getInstance()->resolutionEcoder2;
                    if (App::getInstance()->yNormal == 1) {
                        yDistance = -yDistance;
                    }
                }
            } else if (App::getInstance()->EncoderSwapped == true) { // y_x
                if (Encodemode == EncoderTime::Encoder) {
                    if (Isstart) {
                        EncoderB_pos = TailInform.encoderB_pos;
                        EncoderA_pos = TailInform.encoderA_pos;
                    }
                    Isstart = false;
                    if (xDistance < 0) {
                        xDistance = 0;
                    } else {
                        xDistance = (TailInform.encoderB_pos - EncoderB_pos)
                            / App::getInstance()->resolutionEcoder2
                            * App::getInstance()->currentPosition;
                        if (App::getInstance()->yNormal == 1) {
                            xDistance = -xDistance;
                        }
                    }
                    yDistance = (TailInform.encoderA_pos - EncoderA_pos)
                        / App::getInstance()->resolutionEcoder1;
                    if (App::getInstance()->xNormal == 1) {
                        yDistance = -yDistance;
                    }
                }
            }
        }

        double imgWidth;
        if (Encodemode == EncoderTime::Encoder) {
            imgWidth = App::getInstance()->ScanEnd - App::getInstance()->ScanStart;
            double curPos = App::getInstance()->currentPosition;
            frames = (curPos > 0) ? (imgWidth / curPos) : 1;

        } else {
            imgWidth = App::getInstance()->Scanlength;
            double timeRes = App::getInstance()->timeResolution;
            frames = (timeRes > 0) ? (imgWidth / timeRes) : 1;
        }

        if (frames < 1)
            frames = 1;
        frameNo = qRound((xDistance / imgWidth) * frames);
        frameNo = qBound(0, frameNo, frames - 1);

        local_xDistance = xDistance;
        local_yDistance = yDistance;
        local_frameNo = frameNo;
        local_frames = frames;
    }

    m_totalFramesReceived++;

    // 回放时按编码器列位置更新竖线（每步=扫查分辨率）
    if (App::getInstance()->Replay && m_playbackLine && m_playbackLine->isVisible()
        && !isDragging) {
        setPlaybackSliderValue(local_frameNo);
    }

    // 直接处理数据，不经过队列（对于C扫图）
    if (View == C1_Scan) {
        processFrameData(App::getInstance()->C1, beam, datapacket, false, 0, local_frameNo, local_yDistance);
    } else if (View == C2_Scan) {
        processFrameData(App::getInstance()->C2, beam, datapacket, false, 0, local_frameNo, local_yDistance);
    } else {
        // A扫和E扫仍然使用队列
        RenderData renderData;
        renderData.data = datapacket;
        renderData.beam = beam;
        renderData.replay = false;
        renderData.no = 0;
        renderData.frameNo = local_frameNo;
        renderData.timestamp = QDateTime::currentMSecsSinceEpoch();

        {
            QMutexLocker locker(&m_queueMutex);
            if (m_renderQueue.size() >= m_maxQueueSize) {
                m_renderQueue.dequeue();
                m_framesSkipped++;
            }
            m_renderQueue.enqueue(renderData);
        }

        // 只在没有在途通知时投递一次 GUI 事件；定时器运行期间不再重复投递，
        // 由 renderNextFrame 在停表时复位标志。
        if (!m_notifyQueued.exchange(true)) {
            QMetaObject::invokeMethod(this, [this]() {
                if (!m_renderTimerControl.isActive()) {
                    m_notifyQueued = false;
                    if (m_updateInterval > 0) {
                        m_renderTimerControl.start(m_updateInterval);
                    } else {
                        renderNextFrame();
                    }
                }
            }, Qt::QueuedConnection);
        }
    }
}

void ViewWidget::processFrameData(App::Cscan_Data C_SCAN, int beam, const QByteArray &datapacket,
                                  bool Replay, int No, int frameNo, double yPos)
{
    double scanRes = App::getInstance()->scanResolution;
    int pre = (scanRes > 0) ? static_cast<int>(App::getInstance()->End / scanRes) : 1;
    if (pre < 1)
        pre = 1;
    y1 = yPos;

    bool isAmp = (C_SCAN < 3);

    QMetaObject::invokeMethod(m_worker, "processCScanFrame", Qt::QueuedConnection,
                              Q_ARG(int, frameNo), Q_ARG(int, static_cast<int>(y1)),
                              Q_ARG(int, beam), Q_ARG(QByteArray, datapacket),
                              Q_ARG(int, static_cast<int>(C_SCAN)), Q_ARG(bool, isAmp));

    m_totalFramesProcessed++;
}



void ViewWidget::onAScanReady(QList<QPointF> points)
{
    // 探针：测量 AScan 每次渲染（QtCharts 清空/重建/重绘）耗时
    struct RenderProbe {
        QElapsedTimer t;
        RenderProbe() { t.start(); }
        ~RenderProbe() {
            static qint64 ms = 0;
            static int n = 0;
            ms += t.nsecsElapsed() / 1000;
            n++;
            if (ms >= 500000) {
                qDebug() << "[AScanRenderTiming] onAScanReady平均" << (ms / (double)n)
                         << "µs, n=" << n;
                ms = 0; n = 0;
            }
        }
    } renderProbe;

    if (View != A_Scan)
        return;

    // 调试：A 扫每秒统计渲染帧数和幅值
    static int rFrames = 0;
    static QElapsedTimer rTimer;
    if (!rTimer.isValid())
        rTimer.start();
    rFrames++;
    if (rTimer.elapsed() >= 1000) {
        double first = points.isEmpty() ? -999.0 : points.first().y();
        double last = points.isEmpty() ? -999.0 : points.last().y();
        qDebug() << "[AScan] 渲染:" << rFrames
                 << " 点数:" << points.size()
                 << " 首点%:" << first << " 末点%:" << last;
        rFrames = 0;
        rTimer.restart();
    }

    m_series->clear();
    m_series->replace(points);
    if (m_chartView->scene())
        m_chartView->scene()->invalidate();
    m_chartView->update();

    // 闸门同步时，SI/SA随每帧数据变化，需要刷新闸门位置
    if (m_gateASync == GateSynchron::GateI || m_gateBSync == GateSynchron::GateI
        || m_gateBSync == GateSynchron::GateA || m_gateCSync == GateSynchron::GateI
        || m_gateCSync == GateSynchron::GateA || m_gateCSync == GateSynchron::GateB) {
        showGateView();
    }
}

void ViewWidget::onEScanReady(QImage image)
{
    if (View != E_Scan || !Pix_Label)
        return;

    if (m_cachedPixmap.isNull() || m_cachedPixmap.size() != Pix_Label->size())
        m_cachedPixmap = QPixmap(Pix_Label->size());
    if (!m_cachedPixmap.isNull()) {
        QPainter p(&m_cachedPixmap);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p.drawImage(QRect(0, 0, Pix_Label->width(), Pix_Label->height()), image);
        p.end();
        Pix_Label->setPixmap(m_cachedPixmap);
    }

    m_lastViewSize = Pix_Label->size();
    m_cacheValid = true;
}

void ViewWidget::onCScanImageReady(QImage image)
{
    if (View != C1_Scan && View != C2_Scan)
        return;

    if (!Pix_Label)
        return;

    if (image.isNull())
        return;

    int viewW = width();
    int viewH = height();
    if (viewW < 1) viewW = 1;
    if (viewH < 1) viewH = 1;

    if (!m_cachedPixmap.isNull() && m_cachedPixmap.size() == QSize(viewW, viewH)) {
        QPainter painter(&m_cachedPixmap);
        QImage scaled = image.scaled(viewW, viewH, Qt::IgnoreAspectRatio,
                                      Qt::FastTransformation);
        painter.drawImage(0, 0, scaled);
        painter.end();
        Pix_Label->setPixmap(m_cachedPixmap);
    } else {
        QPixmap pm(QSize(viewW, viewH));
        pm.fill(Qt::black);
        QPainter painter(&pm);
        QImage scaled = image.scaled(viewW, viewH, Qt::IgnoreAspectRatio,
                                      Qt::FastTransformation);
        painter.drawImage(0, 0, scaled);
        painter.end();
        m_cachedPixmap = pm;
        Pix_Label->setPixmap(pm);
    }
    Pix_Label->resize(viewW, viewH);
}

void ViewWidget::sendConfigToWorker()
{
    QMetaObject::invokeMethod(m_worker, "updateConfig", Qt::QueuedConnection,
                              Q_ARG(int, pointQuantity), Q_ARG(double, maxAmp),
                              Q_ARG(int, groupdata_size), Q_ARG(double, range_start),
                              Q_ARG(double, range_end), Q_ARG(double, rePointQuantity));
    App *app = App::getInstance();
    QMetaObject::invokeMethod(m_worker, "updateSoundConfig", Qt::QueuedConnection,
                              Q_ARG(double, app->SoundStart), Q_ARG(double, app->SoundEnd));
    QMetaObject::invokeMethod(m_worker, "setColorPalette", Qt::QueuedConnection,
                              Q_ARG(QVector<QRgb>, color_Amplitude));
}

void ViewWidget::renderNextFrame()
{
    RenderData renderData;
    bool hasData = false;

    {
        QMutexLocker locker(&m_queueMutex);
        if (!m_renderQueue.isEmpty()) {
            if (View == E_Scan || View == A_Scan) {
                // A扫和E扫只渲染最新帧，丢弃中间堆积的帧，避免追赶导致的视觉抖动
                renderData = m_renderQueue.last();
                m_renderQueue.clear();
            } else {
                renderData = m_renderQueue.dequeue();
            }
            hasData = true;
        }
    }

    if (!hasData) {
        if (m_renderTimerControl.isActive()) {
            m_renderTimerControl.stop();
        }
        m_notifyQueued = false;
        return;
    }

    m_renderTimer.start();

    double scanRes = App::getInstance()->scanResolution;
    int pre = (scanRes > 0) ? static_cast<int>(App::getInstance()->End / scanRes) : 1;
    if (pre < 1)
        pre = 1;
    {
        QMutexLocker locker(&m_dataMutex);
        y1 = (scanRes > 0) ? (yDistance / scanRes * renderData.beam) : 0;
    }

    if (View == A_Scan) {
        m_lastAScanData = renderData.data;
        m_lastAScanBeam = renderData.beam;
        m_sider->silder()->setRange(1, renderData.beam);
        QMetaObject::invokeMethod(m_worker, "processAScan", Qt::QueuedConnection,
                                  Q_ARG(QByteArray, renderData.data), Q_ARG(int, renderData.beam),
                                  Q_ARG(int, id_beam), Q_ARG(bool, renderData.replay));
    } else if (View == E_Scan) {
        QMetaObject::invokeMethod(m_worker, "processEScan", Qt::QueuedConnection,
                                  Q_ARG(QByteArray, renderData.data), Q_ARG(int, pointQuantity),
                                  Q_ARG(int, renderData.beam), Q_ARG(bool, renderData.replay));
    }

    updatePerformanceStats(m_renderTimer.elapsed());
    m_framesProcessed++;

    {
        QMutexLocker locker(&m_queueMutex);
        if (!m_renderQueue.isEmpty() && m_renderTimerControl.isActive()) {
        } else if (!m_renderQueue.isEmpty() && m_updateInterval == 0) {
            QMetaObject::invokeMethod(this, &ViewWidget::renderNextFrame, Qt::QueuedConnection);
        } else {
            m_renderTimerControl.stop();
            m_notifyQueued = false;
        }
    }
}

void ViewWidget::showAScan(const QByteArray &b, int cnt_fl)
{
    const int16_t *dataPtr = reinterpret_cast<const int16_t *>(b.constData());
    std::vector<int16_t> imageData;

    auto beam_point = pointQuantity;
    auto cnt_points_1 = beam_point / 8 * 8;

    auto max_size = cnt_fl * cnt_points_1;
    imageData.resize(max_size);
    if (b.size()
        < (groupdata_size + cnt_fl * (beam_point + 16)) * static_cast<long long>(sizeof(int16_t))) {
        return;
    }
    int groupOffset = App::getInstance()->Replay ? 0 : groupdata_size;
    for (int i = 0; i < cnt_fl; i++) {
        for (int j = 0; j < cnt_points_1; j++) {
            imageData[j + i * cnt_points_1] = dataPtr[groupOffset + j + i * (beam_point + 16)];
        }
    }

    // 只在参数变化时重建坐标轴，避免每帧重复创建
    bool axisChanged = !m_axisCacheValid || m_lastAxisRangeStart != range_start
        || m_lastAxisRangeEnd != range_end || m_lastPointQuantity != pointQuantity
        || m_lastRectifierType != reType;

    if (axisChanged) {
        // 更新X轴
        auto xtick_count = 5;
        xAxis->setRange(0, beam_point);
        auto a = AxisInfos::generateAxisInfo(range_start, beam_point, xtick_count,
                                             (range_end - range_start), "mm");
        for (auto &label : xAxis->categoriesLabels()) {
            xAxis->remove(label);
        }
        for (int i = 0; i < xtick_count; i++) {
            xAxis->append(a[i]->label, a[i]->pos);
        }

        for (auto ptr : a) {
            delete ptr;
        }
        a.clear();

        // 检波模式切换刷新A扫图
        auto amp = 100;
        auto amp0 = 0;
        int ytick_count = 11;
        if (reType == RectifierType::Rectifier_RF) {
            amp0 = -100;
            amp = 2 * amp;
            yAxis->setRange(amp0, -amp0);
        } else {
            amp0 = 0;
            amp = 100;
            yAxis->setRange(amp0, amp);
        }

        auto a2 = AxisInfos::generateAxisInfo(amp0, amp, ytick_count, amp, "%");
        for (auto &label : yAxis->categoriesLabels()) {
            yAxis->remove(label);
        }
        for (int i = 0; i < ytick_count; i++) {
            yAxis->append(a2.at(i)->label, a2.at(i)->pos);
        }

        for (auto ptr : a2) {
            delete ptr;
        }
        a2.clear();

        // 更新缓存
        m_lastAxisRangeStart = range_start;
        m_lastAxisRangeEnd = range_end;
        m_lastPointQuantity = pointQuantity;
        m_lastRectifierType = reType;
        m_axisCacheValid = true;
    }

    QList<QPointF> points;
    const int cnt = cnt_points_1;
    int start = (id_beam - 1) * cnt_points_1;
    for (auto i = 0; i < cnt && start < imageData.size(); i++, start++) {
        qreal yValue = 0.0;
        yValue = static_cast<qreal>(imageData[start] * 1.0 / maxAmp * 100);

        QPointF pt(i, yValue);
        points.push_back(pt);
    }
    m_series->clear();
    m_series->replace(points);
    if (m_chartView->scene())
        m_chartView->scene()->invalidate();
    m_chartView->update();

    // 闸门同步时，SI/SA随每帧数据变化，需要刷新闸门位置
    if (m_gateASync == GateSynchron::GateI || m_gateBSync == GateSynchron::GateI
        || m_gateBSync == GateSynchron::GateA || m_gateCSync == GateSynchron::GateI
        || m_gateCSync == GateSynchron::GateA || m_gateCSync == GateSynchron::GateB) {
        showGateView();
    }
}

void ViewWidget::showEScan(int w, int h, const QByteArray &src)
{
    if (w <= 0 || h <= 0) {
        return;
    }

    if (color_Amplitude.isEmpty()) {
        return;
    }

    QSize currentSize = Pix_Label->size();
    if (currentSize.isEmpty()) {
        return;
    }

    int groupOffset = App::getInstance()->Replay ? 0 : groupdata_size;
    if (src.size() < (groupOffset + h * (w + 16)) * static_cast<long long>(sizeof(int16_t))) {
        return;
    }

    // 重用缓存图像，避免每帧重新分配内存
    const int16_t *dataPtr = reinterpret_cast<const int16_t *>(src.constData());
    if (m_cachedEScanImage.width() != h || m_cachedEScanImage.height() != w) {
        m_cachedEScanImage = QImage(h, w, QImage::Format_ARGB32);
    }

    // 从上往下=points 方向 (y 轴)，从左往右=beam 方向 (x 轴)
    // data[beam][point] → image(beam, point)

    for (int i = 0; i < h; i++) {
        int pos1 = i * (w + 16);
        for (int j = 0; j < w; j++) {
            int pos2 = static_cast<int>(fabs(dataPtr[groupOffset + pos1 + j] * 1.0 / maxAmp * 255));
            pos2 = qBound(0, pos2, 255);
            uint32_t *targetLine = reinterpret_cast<uint32_t *>(m_cachedEScanImage.scanLine(j));
            targetLine[i] = color_Amplitude[pos2];
        }
    }

    // 保持持久化显示 QPixmap，用 QPainter 缩放绘制，避免每帧 QPixmap::fromImage + scaled
    if (m_cachedPixmap.isNull() || m_cachedPixmap.size() != currentSize) {
        m_cachedPixmap = QPixmap(currentSize);
    }
    {
        QPainter painter(&m_cachedPixmap);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.drawImage(QRect(0, 0, currentSize.width(), currentSize.height()),
                          m_cachedEScanImage);
        painter.end();
    }
    Pix_Label->setPixmap(m_cachedPixmap);

    m_lastViewSize = currentSize;
    m_cacheValid = true;
}

void ViewWidget::showEScanOptimized(int w, int h, const QByteArray &src)
{
    if (color_Amplitude.isEmpty())
        return;
    if (w <= 0 || h <= 0)
        return;
    if (src.size() < (h * (w + 16) * static_cast<int>(sizeof(int16_t)))) {
        return;
    }

    // 检查缓存是否有效
    if (m_cacheValid && m_cachedEScanImage.width() == w && m_cachedEScanImage.height() == h
        && m_lastLabelSize == Pix_Label->size()) {
        // 使用缓存图像
        Pix_Label->setPixmap(
            QPixmap::fromImage(m_cachedEScanImage).scaled(Pix_Label->width(), Pix_Label->height()));
        return;
    }

    // 更新缓存
    updateImageCache(w, h, src);
}

void ViewWidget::updateImageCache(int w, int h, const QByteArray &src)
{
    // 根据视图类型更新不同的缓存
    if (View == E_Scan) {
        if (color_Amplitude.isEmpty() || w <= 0 || h <= 0)
            return;
        int groupOffset = App::getInstance()->Replay ? 0 : groupdata_size;
        if (src.size() < (groupOffset + h * (w + 16)) * static_cast<long long>(sizeof(int16_t)))
            return;

        // 更新E扫缓存
        const int16_t *dataPtr = reinterpret_cast<const int16_t *>(src.constData());
        if (m_cachedEScanImage.width() != w || m_cachedEScanImage.height() != h) {
            m_cachedEScanImage = QImage(w, h, QImage::Format_ARGB32);
        }

        for (int i = 0; i < h; i++) {
            uint32_t *imgDataLine = reinterpret_cast<uint32_t *>(m_cachedEScanImage.scanLine(i));
            int pos1 = i * (w + 16);
            for (int j = 0; j < w; j++) {
                int pos2 =
                    static_cast<int>(fabs(dataPtr[groupOffset + pos1 + j] * 1.0 / maxAmp * 100));
                if (pos2 > 255)
                    pos2 = 255;
                else if (pos2 < 0)
                    pos2 = 0;
                *imgDataLine = color_Amplitude[pos2];
                imgDataLine++;
            }
        }

        QTransform matrix;
        matrix.rotate(90.0);
        m_cachedEScanImage = m_cachedEScanImage.transformed(matrix, Qt::FastTransformation);

        m_cacheValid = true;
    }
    // 可以添加其他视图的缓存更新逻辑
}

void ViewWidget::updatePerformanceStats(qint64 renderTime)
{
    m_lastRenderTime = renderTime;
    m_totalRenderTime += renderTime;

    // 计算平均渲染时间（滑动平均）
    if (m_framesProcessed > 0) {
        m_avgRenderTime = m_totalRenderTime / m_framesProcessed;
    }

    // 每100帧输出一次性能信息
    if (m_framesProcessed % 100 == 0) {
    }
}

void ViewWidget::setColorPalette(const QVector<QRgb> &colors)
{
    color_Amplitude = colors;
    m_cacheValid = false;
    QMetaObject::invokeMethod(m_worker, "setColorPalette", Qt::QueuedConnection,
                              Q_ARG(QVector<QRgb>, colors));

    if (View == C1_Scan || View == C2_Scan) {
        QMetaObject::invokeMethod(m_worker, "recolorCScan", Qt::QueuedConnection);
    }
}

void ViewWidget::setRenderQuality(RenderQuality quality)
{
    m_renderQuality = quality;

    switch (quality) {
    case Quality_High:
        m_maxQueueSize = 1;   // 不跳过帧
        m_updateInterval = 0; // 立即渲染
        break;
    case Quality_Medium:
        m_maxQueueSize = 3;
        m_updateInterval = 16; // ~60 FPS
        break;
    case Quality_Low:
        m_maxQueueSize = 5;
        m_updateInterval = 33; // ~30 FPS
        break;
    case Quality_Fast:
        m_maxQueueSize = 10;
        m_updateInterval = 66; // ~15 FPS
        break;
    }

    // 设置渲染定时器间隔
    if (m_updateInterval > 0) {
        m_renderTimerControl.setInterval(m_updateInterval);
    }
}

void ViewWidget::showGateView()
{
    ///***************/ 只有A扫图时才显示闸门;
    if (View != SCAN_TYPE::A_Scan) {
        return;
    }

    slot_Rang_startend_ValueChanged();

    QRectF plotArea = m_chart->plotArea();
    qreal yAxisWidth = yAxis->linePen().widthF();
    auto rata = (plotArea.width()) / static_cast<double>(range_end - range_start);
    double Hrata, baseY;
    if (reType == RectifierType::Rectifier_RF) {
        Hrata = plotArea.height() / 200.0;
        baseY = plotArea.top() + plotArea.height() / 2.0;
    } else {
        Hrata = plotArea.height() / 100.0;
        baseY = plotArea.height() + plotArea.top();
    }
    GateAline->hide();
    GateBline->hide();
    GateCline->hide();
    GateIline->hide();

    if (m_gateAEnable) {
        GateAline->show();
        auto wideA = static_cast<double>(m_gateAEnd - m_gateAStart);
        auto heightA = baseY - (m_gateAThreshold * Hrata);
        double startA = 0.0;
        if (m_gateASync == GateSynchron::GateI) {
            auto value = m_gateAStart + App::getInstance()->SI;
            startA = static_cast<double>(value - range_start) * rata;
        } else if (m_gateASync == GateSynchron::Pulser) {
            startA = static_cast<double>(m_gateAStart - range_start) * rata;
        }
        GateAline->setGeometry(plotArea.left() + startA + yAxisWidth, heightA, wideA * rata, 5);
    }
    if (m_gateBEnable) {
        GateBline->show();
        auto wideB = m_gateBEnd - m_gateBStart;
        auto heightB = baseY - (m_gateBThreshold * Hrata);
        double startB = 0.0;
        if (m_gateBSync == GateSynchron::GateI) {
            auto value = m_gateBStart + App::getInstance()->SI;
            startB = static_cast<double>(value - range_start) * rata;
        } else if (m_gateBSync == GateSynchron::GateA) {
            auto value = m_gateBStart + App::getInstance()->SA;
            startB = static_cast<double>(value - range_start) * rata;
        } else if (m_gateBSync == GateSynchron::Pulser) {
            startB = static_cast<double>(m_gateBStart - range_start) * rata;
        }
        GateBline->setGeometry(plotArea.left() + startB + yAxisWidth, heightB, wideB * rata, 5);
    }
    if (m_gateIEnable) {
        GateIline->show();
        auto wideI = m_gateIEnd - m_gateIStart;
        auto heightI = baseY - (m_gateIThreshold * Hrata);
        auto startI = static_cast<double>(m_gateIStart - range_start) * rata;
        GateIline->setGeometry(plotArea.left() + startI + yAxisWidth, heightI, wideI * rata, 5);
    }
    if (m_gateCEnable) {
        GateCline->show();
        auto wideC = static_cast<double>(m_gateCEnd - m_gateCStart);
        auto heightC = baseY - (m_gateCThreshold * Hrata);
        double startC = 0.0;
        if (m_gateCSync == GateSynchron::GateI) {
            auto value = m_gateCStart + App::getInstance()->SI;
            startC = static_cast<double>(value - range_start) * rata;
        } else if (m_gateCSync == GateSynchron::GateA) {
            auto value = m_gateCStart + App::getInstance()->SA;
            startC = static_cast<double>(value - range_start) * rata;
        } else if (m_gateCSync == GateSynchron::GateB) {
            auto value = m_gateCStart + App::getInstance()->SB;
            startC = static_cast<double>(value - range_start) * rata;
        } else if (m_gateCSync == GateSynchron::Pulser) {
            startC = static_cast<double>(m_gateCStart - range_start) * rata;
        }
        GateCline->setGeometry(plotArea.left() + startC + yAxisWidth, heightC, wideC * rata, 5);
    }
}

void ViewWidget::slot_refreshGateStatue()
{
    showGateView();
}

void ViewWidget::slot_GateA_refreshGatedata()
{
    QRectF plotArea = m_chart->plotArea();
    double wrate = (range_end - range_start) / static_cast<double>(m_chartView->width() - 95);

    if (GateAline->isVisible()) {
        double Astart = (GateAline->pos().x() - plotArea.left()) * wrate;
        double Awidth = Astart + GateAline->width() * wrate;
        double Aheight = (m_chartView->height() - GateAline->y() - 50)
            / static_cast<double>(m_chartView->height() - 70) * 100;
        config.setGateAStart(static_cast<int>(Astart));
        config.setGateAEnd(static_cast<int>(Awidth));
        Aheight = Aheight / 100 * maxAmp;

        config.setGateAThreshold(Aheight);
    }
}

void ViewWidget::slot_GateB_refreshGatedata()
{
    QRectF plotArea = m_chart->plotArea();
    double wrate = (range_end - range_start) / static_cast<double>(m_chartView->width() - 95);
    if (GateBline->isVisible()) {
        double Bstart = (GateBline->pos().x() - plotArea.left()) * wrate;
        double Bwidth = Bstart + GateBline->width() * wrate;
        double Bheight = (m_chartView->height() - GateBline->y() - 50)
            / static_cast<double>(m_chartView->height() - 70) * 100;
        config.setGateBStart(static_cast<int>(Bstart));
        config.setGateBEnd(static_cast<int>(Bwidth));

        Bheight = Bheight / 100 * maxAmp;

        config.setGateBThreshold(Bheight);
    }
}

void ViewWidget::slot_GateI_refreshGatedata()
{
    QRectF plotArea = m_chart->plotArea();
    double wrate = (range_end - range_start) / static_cast<double>(m_chartView->width() - 95);
    if (GateIline->isVisible()) {
        double Istart = (GateIline->pos().x() - plotArea.left()) * wrate;
        double Iwidth = Istart + GateIline->width() * wrate;
        double Iheight = (m_chartView->height() - GateIline->y() - 50)
            / static_cast<double>(m_chartView->height() - 70) * 100;
        config.setGateIStart(static_cast<int>(Istart));
        config.setGateIEnd(static_cast<int>(Iwidth));

        Iheight = Iheight / 100 * maxAmp;

        config.setGateIThreshold(Iheight);
    }
}

void ViewWidget::slot_Rang_startend_ValueChanged()
{
    if (View != SCAN_TYPE::A_Scan) {
        return;
    }
}

void ViewWidget::setDragRange(int min, int max)
{
    minRange = min;
    maxRange = max;
    updateBarPosition(currentX);
}

void ViewWidget::setPlaybackSliderRange(int maxFrame)
{
    m_maxFrames = qMax(0, maxFrame);
}

void ViewWidget::resetDragBar()
{
    updateBarPosition(0);
}

void ViewWidget::dragBarShow(bool flag)
{
    if (m_playbackLine)
        m_playbackLine->setVisible(flag);
    if (m_frameLabel)
        m_frameLabel->setVisible(flag);
    if (flag && Pix_Label && m_playbackLine)
        m_playbackLine->setFixedHeight(Pix_Label->height());
}

void ViewWidget::setPlaybackSliderValue(int frame)
{
    if (!m_playbackLine || !m_playbackLine->isVisible() || !Pix_Label)
        return;
    int w = Pix_Label->width();
    if (w <= 0 || m_bufferWidth <= 0)
        return;
    int col = qBound(0, frame, m_dragMaxCol > 0 ? m_dragMaxCol : (m_bufferWidth - 1));
    int xPosCol = qMin(col, m_bufferWidth - 1);
    int xPos = qRound(static_cast<double>(xPosCol) * w / m_bufferWidth);
    m_playbackLine->move(xPos, 0);
    m_playbackLine->setFixedHeight(Pix_Label->height());
    currentX = xPos;

    if (m_frameLabel) {
        m_frameLabel->setText(QString::number(frame));
        m_frameLabel->adjustSize();
        int lx = qBound(0, xPos + 6, Pix_Label->width() - m_frameLabel->width());
        m_frameLabel->move(lx, 4);
    }
}

void ViewWidget::setFrameLabel(int frame)
{
    if (m_frameLabel) {
        m_frameLabel->setText(QString::number(frame));
        m_frameLabel->adjustSize();
        int lx = qBound(0, currentX + 6, Pix_Label->width() - m_frameLabel->width());
        m_frameLabel->move(lx, 4);
    }
}

void ViewWidget::updateBarPosition(int x)
{
    if (m_playbackLine && Pix_Label) {
        x = qBound(0, x, Pix_Label->width());
        m_playbackLine->move(x, 0);
        m_playbackLine->setFixedHeight(Pix_Label->height());
    }
    if (m_frameLabel) {
        int col = (m_bufferWidth > 0 && Pix_Label->width() > 0)
            ? qRound((double)x * m_bufferWidth / Pix_Label->width()) : 0;
        col = qBound(0, col, m_bufferWidth - 1);
        m_frameLabel->setText(QString::number(col));
        m_frameLabel->adjustSize();
        int lx = qBound(0, x + 6, Pix_Label->width() - m_frameLabel->width());
        m_frameLabel->move(lx, 4);
    }
    currentX = x;
}

void ViewWidget::setActive(bool active)
{
    m_isActive = active;
    if (!active) {
        QMutexLocker locker(&m_queueMutex);
        m_renderQueue.clear();
        m_renderTimerControl.stop();
        m_notifyQueued = false;
    }
}

void ViewWidget::setUpdateInterval(int ms)
{
    m_updateInterval = ms;
    if (m_updateInterval > 0) {
        m_renderTimerControl.setInterval(m_updateInterval);
    }
}

void ViewWidget::mousePressEvent(QMouseEvent *event)
{
    if (App::getInstance()->Replay && m_playbackLine && m_playbackLine->isVisible()
        && event->button() == Qt::LeftButton) {
        isDragging = true;
        dragOffset = event->pos().x() - m_playbackLine->x();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ViewWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (isDragging && m_playbackLine && m_playbackLine->isVisible() && Pix_Label) {
        int newX = qBound(0, event->pos().x() - dragOffset, Pix_Label->width());
        updateBarPosition(newX);

        // 拖拽位置映射到C扫列号（不超最终帧列）
        if (m_bufferWidth > 0 && Pix_Label->width() > 0) {
            int col = qRound(static_cast<double>(newX) * m_bufferWidth / Pix_Label->width());
            col = qBound(0, col, m_bufferWidth - 1);
            emit App::getInstance()->signal_frameChanged(col);

            // 直接从像素位置计算物理位置，与C扫下方刻度尺保持一致
            double ratio = static_cast<double>(newX) / Pix_Label->width();
            double pos;
            if (Encodemode == EncoderTime::Encoder) {
                pos = App::getInstance()->ScanStart
                    + ratio * (App::getInstance()->ScanEnd - App::getInstance()->ScanStart);
            } else {
                pos = ratio * App::getInstance()->Scanlength;
            }
            emit App::getInstance()->signal_xPosition(pos);
            emit playbackFrameChanged(col);
        }
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void ViewWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
        if (App::getInstance()->Replay && m_playbackLine && m_bufferWidth > 0) {
            int col = qRound((double)m_playbackLine->x() * m_bufferWidth / Pix_Label->width());
            col = qBound(0, col, m_bufferWidth - 1);
            emit playbackFrameChanged(col);
        }
    }
    QWidget::mouseReleaseEvent(event);
}

void ViewWidget::onViewResized()
{
    m_cacheValid = false;      // 使缓存失效
    m_lastLabelSize = QSize(); // 重置标签尺寸
}

// 颜色样式
void ViewWidget::setChartColors(const QColor &bgColor, const QColor &lineColor)
{
    if (m_chart) {
        m_chart->setBackgroundBrush(QBrush(bgColor));
        m_chart->setPlotAreaBackgroundVisible(true);
        m_chart->setPlotAreaBackgroundBrush(QBrush(bgColor));
    }
    if (m_series) {
        QPen pen(lineColor);
        pen.setWidthF(1.5);
        m_series->setPen(pen);
    }
}

// 坐标轴
void ViewWidget::setAxisColor(const QColor &color)
{
    if (!m_chart)
        return;

    // 获取所有坐标轴
    QList<QAbstractAxis *> axes = m_chart->axes();
    for (QAbstractAxis *axis : axes) {
        axis->setLabelsColor(color);
        axis->setTitleBrush(QBrush(color));

        if (QValueAxis *valueAxis = qobject_cast<QValueAxis *>(axis)) {
            QPen pen(color);
            pen.setWidth(1);
            valueAxis->setLinePen(pen);

            // 设置网格线颜色
            QPen gridPen(color);
            gridPen.setWidth(1);
            valueAxis->setGridLinePen(gridPen);

        } else if (QCategoryAxis *categoryAxis = qobject_cast<QCategoryAxis *>(axis)) {
            QPen pen(color);
            pen.setWidth(1);
            categoryAxis->setLinePen(pen);
            categoryAxis->setGridLinePen(QPen(color));
        }
    }
}

// 清空A扫
void ViewWidget::clearSeries()
{
    if (m_series) {
        m_series->clear();
    }
}
