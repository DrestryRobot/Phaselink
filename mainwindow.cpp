#include "mainwindow.h"

#include <app.h>

#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QStyleOptionTitleBar>
#include <algorithm>
#include <cmath>

#include "dialog/AddDeviceDialog.h"
#include "ui_mainwindow.h"
#include <QPushButton>

namespace {
    DataProcessor *g_dataProcessor = nullptr;
    std::atomic<int> g_dataSeq { 0 };

    void dataPacketCallback(const char *data, int length, int deviceId)
    {
        if (!g_dataProcessor)
            return;
        QByteArray qba(data, length);
        delete[] data;
        int seq = g_dataSeq.load();
        QMetaObject::invokeMethod(g_dataProcessor, [=]() {
            if (seq == g_dataSeq.load())
                g_dataProcessor->enqueueData(qba, deviceId);
        }, Qt::QueuedConnection);
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , config(Client::getInstance())

{
    ui->setupUi(this);
    initWidget();
    initSlot();
    m_dataProcessor->start();
}

MainWindow::~MainWindow()
{
    // 1. 先移除数据回调，防止销毁过程中有新数据到达
    config.removeDataPacketCallback();

    // 2. 停止数据采集并断开设备连接
    config.startCapture(false);
    for (auto it = m_deviceStates.begin(); it != m_deviceStates.end(); ++it) {
        if (it.value() == ConState::Connected) {
            config.Disconnect();
            break;
        }
    }

    // 3. 等待SDK内部线程完成收尾工作
    for (int i = 0; i < 50; i++) {
        QThread::msleep(2);
    }

    // 4. 停止回放定时器
    if (m_playbackTimer) {
        m_playbackTimer->stop();
    }

    // 5. 停止数据处理线程
    if (m_dataProcessor) {
        m_dataProcessor->stop();
        for (int i = 0; i < 100 && m_dataProcessor->isProcessing(); i++) {
            QThread::msleep(1);
        }
    }
    if (m_processorThread) {
        m_processorThread->quit();
        m_processorThread->wait(3000);
        delete m_dataProcessor;
        m_dataProcessor = nullptr;
        g_dataProcessor = nullptr;
    }

    delete m_dataSaver;
    delete ui;
}
void MainWindow::initWidget()
{
    for (int i = 0; i < ui->Top_funct_btnGroup->buttons().size(); i++) {
        ui->Top_funct_btnGroup->setId(ui->Top_funct_btnGroup->buttons().at(i), i);
    }
    m_processorThread = new QThread(this);
    m_processorThread->setObjectName(QStringLiteral("DataProcessorThread"));
    m_dataProcessor = new DataProcessor();
    m_dataProcessor->moveToThread(m_processorThread);
    g_dataProcessor = m_dataProcessor;
    m_processorThread->start();
    // ui->connectBtn->setEnabled(false);

    refreshViewSize();

    ui->View_1->setScanView(ViewWidget::A_Scan);
    ui->View_2->setScanView(ViewWidget::E_Scan);
    ui->View_2->setColorPalette(ui->S_Scan_Color->getColors());
    ui->View_3->setScanView(ViewWidget::C1_Scan);
    ui->View_3->setColorPalette(ui->Amp_color->getColors());
    ui->View_3->dragBarShow(false);

    ui->View_4->setScanView(ViewWidget::C2_Scan);
    ui->View_4->setColorPalette(ui->Sound_Color->getColors());
    ui->ruler_widget->setPosStart(App::getInstance()->ScanStart);
    ui->ruler_widget->setPosEnd(App::getInstance()->ScanEnd);
    ui->ruler_Sound->setPosStart(App::getInstance()->ScanStart);
    ui->ruler_Sound->setPosEnd(App::getInstance()->ScanEnd);
    ui->ruler_range->setDirections(Directions::Vertical_left);
    ui->ruler_range->setPosStart(0.0);
    ui->ruler_range->setPosEnd(50.0);

    double end = (config.getBeamCounts() - 1) * 0.6 * 1;
    ui->ruler_distance_s->setPosStart(0);
    ui->ruler_distance_s->setPosEnd(end);

    ui->ruler_distance_c1->setDirections(Directions::Vertical_left);
    ui->ruler_distance_c1->setPosStart(0);
    ui->ruler_distance_c1->setPosEnd(end);

    ui->ruler_distance_c2->setDirections(Directions::Vertical_left);
    ui->ruler_distance_c2->setPosStart(0);
    ui->ruler_distance_c2->setPosEnd(end);

    ui->Amp_ruler->setDirections(Directions::Vertical_right);
    ui->Amp_ruler->setPosStart(0);
    ui->Amp_ruler->setPosEnd(100);
    ui->Amp_ruler->setRulerUnit("%");

    ui->Sound_ruler->setDirections(Directions::Vertical_right);
    ui->Sound_ruler->setPosStart(App::getInstance()->SoundStart);
    ui->Sound_ruler->setPosEnd(App::getInstance()->SoundEnd);

    ui->comboBox->setView(new QListView);
    ui->viewComboBox->setView(new QListView);
    ui->comboBox_ip->setView(new QListView);
    ui->comboBox_id->setView(new QListView);
    ui->SaveData->setText(tr("Start Save"));
    ui->SaveData->setEnabled(false);
    ui->Replay->setEnabled(true);

    QTimer::singleShot(2900, this, [this] {
        if (!m_dataProcessor)
            return;
        // 初始化设备列表
        refreshDeviceComboBox();
    });

    m_dataSaver = new PacketDataSaver(this);
    m_playbackTimer = new QTimer(this);
    m_playbackTimer->setInterval(33);

    connect(m_playbackTimer, &QTimer::timeout, this, [this]() {
        if (m_playbackKeys.isEmpty())
            return;
        m_currentPlaybackIdx++;
        if (m_currentPlaybackIdx >= m_playbackKeys.size()) {
            m_playbackTimer->stop();
            m_currentPlaybackIdx = m_playbackKeys.size() - 1;
            return;
        }
        sendPlaybackFrame(m_currentPlaybackIdx);
    });

    connect(App::getInstance(), &App::signal_frameChanged, this, [this](int col) {
        if (!App::getInstance()->Replay || m_playbackKeys.isEmpty())
            return;
        // 拖拽竖线传入C扫列号，映射到最接近的录制帧
        int bufW = ui->View_3->bufferWidth();
        int idx = bufW > 0 ? col * m_playbackKeys.size() / bufW : 0;
        idx = qBound(0, idx, m_playbackKeys.size() - 1);
        m_currentPlaybackIdx = idx;
        sendPlaybackFrame(idx);
    });
}

void MainWindow::initSlot()
{
    connect(ui->Top_funct_btnGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            this, &MainWindow::slot_pushButton_click);

    // 注册数据回调
    config.setDataPacketCallback(dataPacketCallback);
        m_dataProcessor->setViews(ui->View_1, ui->View_2, ui->View_3, ui->View_4, ui->measure_widget);

    connect(App::getInstance(), &App::signal_Reset_CScan, this, [=](int index) {
        if (index == 0) {
            ui->ruler_widget->setRulerUnit("mm");
            ui->ruler_widget->setPosStart(App::getInstance()->ScanStart);
            ui->ruler_widget->setPosEnd(App::getInstance()->ScanEnd);
            ui->ruler_Sound->setRulerUnit("mm");
            ui->ruler_Sound->setPosStart(App::getInstance()->ScanStart);
            ui->ruler_Sound->setPosEnd(App::getInstance()->ScanEnd);
        } else {
            ui->ruler_widget->setRulerUnit(" s");
            ui->ruler_widget->setPosStart(0);
            ui->ruler_widget->setPosEnd(App::getInstance()->Scanlength);
            ui->ruler_Sound->setRulerUnit(" s");
            ui->ruler_Sound->setPosStart(0);
            ui->ruler_Sound->setPosEnd(App::getInstance()->Scanlength);
        }
    });

    connect(App::getInstance(), &App::signal_soundChanged, this, [=]() {
        ui->Sound_ruler->setPosStart(App::getInstance()->SoundStart);
        ui->Sound_ruler->setPosEnd(App::getInstance()->SoundEnd);
    });

    connect(App::getInstance(), &App::signal_RangeChanged, this,
            &MainWindow::slot_rulerWidgetChanged);

    connect(App::getInstance(), &App::signal_probeChange, this,
            &MainWindow::slot_rulerProbeChanged);
    connect(App::getInstance(), &App::signal_paletteChanged, this, [=] {
        ui->View_2->setColorPalette(ui->S_Scan_Color->getColors());
        ui->View_3->setColorPalette(ui->Amp_color->getColors());
        ui->View_4->setColorPalette(ui->Sound_Color->getColors());
    });

    connect(ui->disconnect_btn, &QPushButton::clicked, this, &MainWindow::DisconnectClick);

    // 初始化设备状态
    refreshDeviceComboBox();
}

DeviceKey MainWindow::getCurrentDeviceKey()
{
    DeviceKey key;
    key.ip = ui->comboBox_ip->currentText();
    key.deviceId = ui->comboBox_id->currentText().toInt();
    return key;
}

void MainWindow::refreshDeviceComboBox()
{
    // 保存当前选中的值
    QString currentIp = ui->comboBox_ip->currentText();
    int currentId = ui->comboBox_id->currentText().toInt();

    // 获取所有服务器
    auto servers = config.getAllServers();

    // 收集唯一的IP
    QSet<QString> uniqueIps;
    for (const auto &server : servers) {
        uniqueIps.insert(QString::fromStdString(server.address));
    }

    // 获取当前下拉框中已有的IP
    QSet<QString> existingIps;
    for (int i = 0; i < ui->comboBox_ip->count(); ++i) {
        existingIps.insert(ui->comboBox_ip->itemText(i));
    }

    // 添加新的IP（只添加不存在的）
    for (const QString &ip : uniqueIps) {
        if (!existingIps.contains(ip)) {
            ui->comboBox_ip->addItem(ip);
        }
    }

    // 移除已经不存在的IP
    for (int i = ui->comboBox_ip->count() - 1; i >= 0; --i) {
        QString ip = ui->comboBox_ip->itemText(i);
        if (!uniqueIps.contains(ip)) {
            ui->comboBox_ip->removeItem(i);
        }
    }

    // 恢复选中的IP
    int ipIndex = ui->comboBox_ip->findText(currentIp);
    if (ipIndex >= 0) {
        ui->comboBox_ip->setCurrentIndex(ipIndex);
    } else if (ui->comboBox_ip->count() > 0) {
        ui->comboBox_ip->setCurrentIndex(0);
    }

    // 根据当前IP更新设备ID下拉框
    updateDeviceComboBoxForIp(ui->comboBox_ip->currentText());

    // 恢复选中的设备ID
    int idIndex = ui->comboBox_id->findText(QString::number(currentId));
    if (idIndex >= 0) {
        ui->comboBox_id->setCurrentIndex(idIndex);
    } else if (ui->comboBox_id->count() > 0) {
        ui->comboBox_id->setCurrentIndex(0);
    }

    // 更新当前选中的设备
    m_currentSelectedDevice = getCurrentDeviceKey();
    updateConnectButtonState();
}

void MainWindow::updateDeviceComboBoxForIp(const QString &ip)
{
    // 收集该IP下的所有设备ID
    QSet<int> validIds;
    auto servers = config.getAllServers();
    for (const auto &server : servers) {
        if (QString::fromStdString(server.address) == ip) {
            validIds.insert(server.serverId);
        }
    }

    // 获取当前下拉框中已有的ID
    QSet<int> existingIds;
    for (int i = 0; i < ui->comboBox_id->count(); ++i) {
        existingIds.insert(ui->comboBox_id->itemText(i).toInt());
    }

    // 添加新的ID（只添加不存在的）
    for (int id : validIds) {
        if (!existingIds.contains(id)) {
            ui->comboBox_id->addItem(QString::number(id));
        }
    }

    // 移除已经不存在的ID
    for (int i = ui->comboBox_id->count() - 1; i >= 0; --i) {
        int id = ui->comboBox_id->itemText(i).toInt();
        if (!validIds.contains(id)) {
            ui->comboBox_id->removeItem(i);
        }
    }

    // 如果同一个IP下有多个ID（comboBox_id项目数 > 1），禁用Add和Remove按钮
    bool hasMultipleIds = (ui->comboBox_id->count() > 1);
    ui->BtnAdd->setEnabled(!hasMultipleIds);
    ui->BtnRemove->setEnabled(!hasMultipleIds);
}

void MainWindow::updateConnectButtonState()
{
    DeviceKey currentDevice = getCurrentDeviceKey();

    if (currentDevice.ip.isEmpty()) {
        ui->connectBtn->setText(tr("Connect"));
        ui->connectBtn->setStyleSheet("QPushButton{background:#A3C8F0;color:#000000;}");
        ui->connectBtn->setEnabled(true);
        return;
    }

    ConState state = m_deviceStates.value(currentDevice, ConState::Unknown);

    switch (state) {
    case ConState::Connected:
        ui->connectBtn->setText(tr("Connecting"));
        ui->connectBtn->setStyleSheet("QPushButton{background:#1BA660;color:#ffffff;}");
        ui->connectBtn->setEnabled(false);
        break;
    case ConState::Unconnected:
        ui->connectBtn->setText(tr("Disconnect"));
        ui->connectBtn->setStyleSheet("QPushButton{background:#A3C8F0;color:#000000;}");
        ui->connectBtn->setEnabled(true);
        break;
    case ConState::Failed:
        ui->connectBtn->setText(tr("Failed"));
        ui->connectBtn->setStyleSheet("QPushButton{background:#ff0000;color:#ffffff;}");
        ui->connectBtn->setEnabled(true);
        break;
    case ConState::Unknown:
        ui->connectBtn->setText(tr(""));
        ui->connectBtn->setStyleSheet("QPushButton{background:#A3C8F0;color:#000000;}");
        ui->connectBtn->setEnabled(true);
        break;
    }
}

void MainWindow::on_connect_btn_clicked()
{
    DeviceKey currentDevice = getCurrentDeviceKey();
    if (currentDevice.ip.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No device selected!"));
        return;
    }

    ConState currentState = m_deviceStates.value(currentDevice, ConState::Unconnected);

    if (currentState == ConState::Connected) {
        QMessageBox::information(
            this, tr("Info"), tr("Device %1 is already connected!").arg(currentDevice.toString()));
        return;
    }

    if (currentState == ConState::Failed) {
        m_deviceStates[currentDevice] = ConState::Unconnected;
    }

    auto ser = config.getAllServers();
    for (auto server : ser) {
        if (server.address == currentDevice.ip.toStdString()
            && server.serverId == currentDevice.deviceId) {
            config.setCurrentServerId(server.serverId);
            break;
        }
    }

    bool flag = config.Connect();

    if (flag) {
        config.startCapture(false);
        m_deviceStates[currentDevice] = ConState::Connected;
        updateConnectButtonState();

        ui->View_3->initCsacnView();
        ui->View_4->initCsacnView();
        ui->View_3->Isstart = true;

        QTimer::singleShot(500, this, [] {
            emit App::getInstance()->refresh_Allpara();
            emit App::getInstance()->signal_GateView_Refresh();
            emit App::getInstance()->signal_RangeChanged();
            emit App::getInstance()->signal_Connected();
            emit App::getInstance()->signal_tcgChanged();
        });

        // 每次连接都确保数据处理器在运行（回放后stop了需要重启）
        m_dataProcessor->start();

        QMessageBox::information(this, tr("Success"),
                                 tr("Connected to device %1").arg(currentDevice.toString()));
    } else {
        m_deviceStates[currentDevice] = ConState::Failed;
        updateConnectButtonState();
        QMessageBox::warning(this, tr("Error"),
                             tr("Failed to connect to device %1").arg(currentDevice.toString()));
    }
}

void MainWindow::DisconnectDevice(const DeviceKey &device)
{
    if (config.Disconnect()) {
        m_deviceStates[device] = ConState::Unconnected;

        // 如果断开的是当前选中的设备，更新按钮状态
        if (m_currentSelectedDevice == device) {
            updateConnectButtonState();
        }

        QMessageBox::information(this, tr("Info"),
                                 tr("Disconnected from device %1").arg(device.toString()));
    } else {
        QMessageBox::warning(this, tr("Error"),
                             tr("Failed to disconnect from device %1").arg(device.toString()));
    }
}

void MainWindow::DisconnectClick()
{
    DeviceKey currentDevice = getCurrentDeviceKey();
    if (!currentDevice.ip.isEmpty()) {
        DisconnectDevice(currentDevice);
    }
}
void MainWindow::resizeEvent(QResizeEvent *e)
{
    refreshViewSize();
}

void MainWindow::changeEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        if (IsConnect == ConState::Unconnected) {
            ui->connectBtn->setText(tr("Unconnected"));
        } else if (IsConnect == ConState::Connected) {
            ui->connectBtn->setText(tr("Connecting"));
        } else {
            ui->connectBtn->setText(tr("Failed"));
        }
        ui->lineEdit->setText(QString::fromStdString(ipAddress));
        break;
    default:
        break;
    }
    QWidget::changeEvent(event);
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    ipAddress = config.getCurrentServerAddr();
    ui->lineEdit->setText(QString::fromStdString(ipAddress));
}

void MainWindow::slot_pushButton_click(QAbstractButton *btn)
{
    auto Num = btn->group()->checkedId();
    if (Num > 1) {
        ui->add_sud_widget->hide();
    } else {
        ui->add_sud_widget->show();
    }
    ui->para_stackedWidget->setCurrentIndex(Num);
}

void MainWindow::on_startscan_btn_clicked()
{
    config.startCapture(true);
    m_dataProcessor->resetRecording();
    ui->SaveData->setEnabled(true);
}

void MainWindow::on_stopscan_btn_clicked()
{
    config.startCapture(false);
}

void MainWindow::on_Origin_btn_clicked()
{
    config.resetCapture();
    m_playbackTimer->stop();

    QTimer::singleShot(100, this, [=]()
    {
        ui->View_3->initCsacnView();
        ui->View_4->initCsacnView();
        ui->View_3->Isstart = true;
        ui->View_4->Isstart = true;
    });
    ui->startscan_btn->setEnabled(true);
    ui->connect_btn->setCheckable(true);
    ui->View_3->dragBarShow(false);
    ui->View_3->resetDragBar();
    ui->Replay->setEnabled(false);
    App::getInstance()->Replay = false;
    {
        QMutexLocker locker(&App::getInstance()->recordBufferMutex);
        App::getInstance()->recordBuffer.clear();
    }
    m_dataProcessor->resetRecording();
    m_playbackFrames.clear();
    m_playbackKeys.clear();
    emit
}

void MainWindow::refreshViewSize()
{
    if (ui->viewComboBox->currentIndex() == 0) {
        ui->View_1->setMaximumWidth(ui->view_widget->width() / 2);
        ui->S_Scan->setMaximumWidth(ui->view_widget->width() / 2);
        ui->View_1->setMaximumHeight((this->height() - 160));
        ui->S_Scan->setMaximumHeight((this->height() - 160));
        ui->S_Scan->show();
        ui->C_Scan->hide();
        ui->C2_Scan->hide();
    } else if (ui->viewComboBox->currentIndex() == 1) {
        ui->C_Scan->show();
        ui->C2_Scan->hide();

        ui->View_1->setMaximumWidth(ui->view_widget->width() / 2);
        ui->S_Scan->setMaximumWidth(ui->view_widget->width() / 2);
        ui->C_Scan->setMinimumWidth(ui->view_widget->width());

        ui->View_1->setMaximumHeight((this->height() - 160) / 2);
        ui->S_Scan->setMaximumHeight((this->height() - 160) / 2);
        ui->C_Scan->setMaximumHeight((this->height() - 160) / 2);

    } else if (ui->viewComboBox->currentIndex() == 2) {
        ui->C_Scan->show();
        ui->C2_Scan->show();

        ui->View_1->setMaximumWidth(ui->view_widget->width() / 2);
        ui->S_Scan->setMaximumWidth(ui->view_widget->width() / 2);
        ui->C_Scan->setMinimumWidth(ui->view_widget->width());
        ui->C2_Scan->setMinimumWidth(ui->view_widget->width());

        ui->View_1->setMaximumHeight((this->height() - 160) / 3);
        ui->S_Scan->setMaximumHeight((this->height() - 160) / 3);
        ui->C_Scan->setMaximumHeight((this->height() - 160) / 3);
        ui->C2_Scan->setMaximumHeight((this->height() - 160) / 3);
    }
    // 通知视图尺寸已变化
    ui->View_1->onViewResized();
    ui->View_2->onViewResized();
    ui->View_3->onViewResized();
    ui->View_4->onViewResized();
}

void MainWindow::on_comboBox_currentIndexChanged(int index)
{
    if (index) {
        lang.load(QCoreApplication::applicationDirPath() + "/phaselink_EN.qm");
    } else {
        lang.load(QCoreApplication::applicationDirPath() + "/phaselink_CN.qm");
    }
    qApp->installTranslator(&lang);
}

void MainWindow::on_comboBox_ip_currentIndexChanged(int index)
{
    Q_UNUSED(index);

    QString currentIp = ui->comboBox_ip->currentText();
    updateDeviceComboBoxForIp(currentIp);

    // 更新当前选中的设备
    m_currentSelectedDevice = getCurrentDeviceKey();
    updateConnectButtonState();

    // 设置服务器地址
    if (!m_currentSelectedDevice.ip.isEmpty()) {
        auto servers = config.getAllServers();
        for (const auto &server : servers) {
            if (server.serverId == m_currentSelectedDevice.deviceId) {
                config.setCurrentServerId(server.serverId);
                break;
            }
        }
    }

    // 如果设备已连接，刷新参数
    if (!m_currentSelectedDevice.ip.isEmpty()
        && m_deviceStates.value(m_currentSelectedDevice) == ConState::Connected) {
        emit App::getInstance()->refresh_Allpara();
        emit App::getInstance()->signal_GateView_Refresh();
        emit App::getInstance()->signal_RangeChanged();
        emit App::getInstance()->signal_Connected();
    }

    // 重置视图
    ui->View_3->initCsacnView();
    ui->View_4->initCsacnView();
    ui->View_3->Isstart = true;
    ui->View_4->Isstart = true;
    ui->View_3->dragBarShow(false);
    ui->View_3->resetDragBar();
}

void MainWindow::on_comboBox_id_currentIndexChanged(int index)
{
    Q_UNUSED(index);

    // 更新当前选中的设备
    m_currentSelectedDevice = getCurrentDeviceKey();
    updateConnectButtonState();

    // 设置服务器地址
    if (!m_currentSelectedDevice.ip.isEmpty()) {
        auto servers = config.getAllServers();
        for (const auto &server : servers) {
            if (server.serverId == m_currentSelectedDevice.deviceId) {
                config.setCurrentServerId(server.serverId);
                break;
            }
        }
    }

    // 如果设备已连接，刷新参数
    if (!m_currentSelectedDevice.ip.isEmpty()
        && m_deviceStates.value(m_currentSelectedDevice) == ConState::Connected) {
        emit App::getInstance()->refresh_Allpara();
        emit App::getInstance()->signal_GateView_Refresh();
        emit App::getInstance()->signal_RangeChanged();
        emit App::getInstance()->signal_Connected();
    }
}

void MainWindow::on_viewComboBox_currentIndexChanged(int index)
{
    refreshViewSize();
    ui->View_3->initCsacnView();
    ui->View_4->initCsacnView();

    ui->View_3->Isstart = true;
    ui->View_4->Isstart = true;
}

void MainWindow::on_lineEdit_editingFinished()
{
    QString ip = ui->lineEdit->text().trimmed(); // 去除首尾空格
    int id = ui->comboBox_id->currentText().toInt();

    if (ip.isEmpty() || id < 0) {
        ui->lineEdit->setText(ui->comboBox_ip->currentText());
        return;
    }

    config.setServerAddr(ip.toStdString(), id);
    refreshDeviceComboBox();
}

void MainWindow::slot_rulerWidgetChanged()
{
    ui->ruler_range->setRulerUnit("mm");
    ui->ruler_range->setDirections(Directions::Vertical_left);
    ;
    ui->ruler_range->setPosStart(config.getRangeStart());
    ui->ruler_range->setPosEnd(config.getRangeEnd());
}

void MainWindow::slot_rulerProbeChanged()
{
    auto num =
        config.getBeamLastElement() - config.getBeamFirstElement() - config.getBeamAperture() + 1;
    if (num < 1)
        num = 1;
    int step = config.getBeamElementStep();
    if (step < 1)
        step = 1;
    num = num / step + 1;
    double end = (num - 1) * config.getProbePrimaryElementsPitch() * step;
    ui->ruler_distance_s->setPosStart(0);
    ui->ruler_distance_s->setPosEnd(end);

    ui->ruler_distance_c1->setDirections(Directions::Vertical_left);
    ;
    ui->ruler_distance_c1->setPosStart(0);
    ui->ruler_distance_c1->setPosEnd(end);

    ui->ruler_distance_c2->setDirections(Directions::Vertical_left);
    ;
    ui->ruler_distance_c2->setPosStart(0);
    ui->ruler_distance_c2->setPosEnd(end);
}

void MainWindow::on_SaveData_clicked()
{
    if (!m_dataProcessor->isSavingEnabled()) {
        // 开始录制：清空旧缓冲区
        {
            QMutexLocker locker(&App::getInstance()->recordBufferMutex);
            App::getInstance()->recordBuffer.clear();
        }
        m_dataProcessor->resetRecording();
        m_dataProcessor->setSavingEnabled(true);
        ui->SaveData->setText(tr("Save to File"));
        return;
    }

    // 停止录制，保存到文件
    m_dataProcessor->setSavingEnabled(false);
    ui->SaveData->setText(tr("Start Save"));

    QMap<quint32, QByteArray> bufferSnapshot;
    {
        QMutexLocker locker(&App::getInstance()->recordBufferMutex);
        if (App::getInstance()->recordBuffer.isEmpty()) {
            QMessageBox::warning(this, tr("Warning"), tr("No data to save!"));
            return;
        }
        bufferSnapshot = App::getInstance()->recordBuffer;
    }

    QString fileName = QFileDialog::getSaveFileName(
        this, tr("Save Recording"),
        QDir::homePath()
            + QString("/scan_%1.pdata")
                  .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
        tr("Phaselink Data (*.pdata);;All Files (*)"));
    if (fileName.isEmpty())
        return;

    m_dataSaver->setData(bufferSnapshot);
    int beamCount = config.getBeamCounts();
    int points = config.getPointQuantity();
    if (m_dataSaver->saveToFile(fileName, beamCount, points)) {
        QMessageBox::information(this, tr("Info"), tr("Recording saved to:\n%1").arg(fileName));
        ui->Replay->setEnabled(true);
    } else {
        QMessageBox::warning(this, tr("Warning"), tr("Failed to save recording!"));
    }
}

void MainWindow::on_Replay_clicked()
{
    // 只有在离线（未连接设备）时才能回放
    for (auto it = m_deviceStates.constBegin(); it != m_deviceStates.constEnd(); ++it) {
        if (it.value() == ConState::Connected) {
            QMessageBox::warning(this, tr("Warning"),
                                 tr("Please disconnect or restart the device before replay!"));
            return;
        }
    }

    try {
        config.startCapture(false);
    } catch (...) {
    }

    m_dataProcessor->stop();
    m_playbackTimer->stop();
    {
        QMutexLocker locker(&App::getInstance()->recordBufferMutex);
        App::getInstance()->recordBuffer.clear();
    }
    m_dataProcessor->resetRecording();

    QString filePath = QFileDialog::getOpenFileName(this, tr("选择录制文件"), QString(),
                                                    tr("Phaselink Data (*.pdata);;All Files (*)"));
    if (filePath.isEmpty()) {
        m_dataProcessor->start();
        return;
    }

    m_playbackFrames.clear();
    m_playbackKeys.clear();
    m_currentPlaybackIdx = 0;

    if (!m_dataSaver->loadFromFile(filePath)) {
        QMessageBox::warning(this, tr("Warning"), tr("Failed to load recording!"));
        m_dataProcessor->start();
        return;
    }

    m_playbackFrames = m_dataSaver->getData();
    m_playbackKeys = m_playbackFrames.keys();
    std::sort(m_playbackKeys.begin(), m_playbackKeys.end());

    if (m_playbackKeys.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No frame data found in file!"));
        m_dataProcessor->start();
        return;
    }

    App::getInstance()->Replay = true;

    // Step 1: 先刷新所有参数（UI组件从Client读取录制配置）
    emit App::getInstance()->refresh_Allpara();
    emit App::getInstance()->signal_GateView_Refresh();
    emit App::getInstance()->signal_RangeChanged();
    emit App::getInstance()->signal_Connected();
    emit App::getInstance()->signal_tcgChanged();

    // Step 2: 用刷新后的参数初始化C扫缓冲区
    ui->View_3->initCsacnView();
    ui->View_4->initCsacnView();
    ui->View_3->Isstart = true;
    ui->View_4->Isstart = true;

    // Step 3: 显示C扫竖直指示线（拖拽不超最后一帧的列）
    ui->View_3->resetDragBar();
    ui->View_3->setPlaybackSliderRange(m_playbackKeys.size() - 1);
    {
        // 计算最后一帧的编码器列号作为拖拽上限
        QByteArray firstData = m_playbackFrames.value(m_playbackKeys.first());
        QByteArray lastData = m_playbackFrames.value(m_playbackKeys.last());
        int lastCol = ui->View_3->bufferWidth() - 1;
        if (firstData.size() >= (int)sizeof(datapacketTail)
            && lastData.size() >= (int)sizeof(datapacketTail)) {
            datapacketTail firstTail, lastTail;
            memcpy(&firstTail, firstData.data() + firstData.size() - sizeof(firstTail),
                   sizeof(firstTail));
            memcpy(&lastTail, lastData.data() + lastData.size() - sizeof(lastTail),
                   sizeof(lastTail));
            int32_t enc = lastTail.encoderA_pos, firstEnc = firstTail.encoderA_pos;
            double res = App::getInstance()->resolutionEcoder1;
            if (App::getInstance()->EncoderSwapped) {
                enc = lastTail.encoderB_pos;
                firstEnc = firstTail.encoderB_pos;
                res = App::getInstance()->resolutionEcoder2;
            }
            double dist = (enc - firstEnc) * App::getInstance()->currentPosition / res;
            if (App::getInstance()->xNormal == 1)
                dist = -dist;
            if (dist < 0)
                dist = 0;
            double imgW = App::getInstance()->ScanEnd - App::getInstance()->ScanStart;
            int numCols = qMax(1, (int)(imgW / App::getInstance()->currentPosition));
            lastCol = qBound(0, (int)(dist / imgW * numCols), numCols - 1);
        }
        ui->View_3->m_dragMaxCol = lastCol;
    }
    ui->View_3->dragBarShow(true);

    // Step 4: 发送第一帧
    m_currentPlaybackIdx = 0;
    sendPlaybackFrame(0);
    m_playbackTimer->start();
}

void MainWindow::sendPlaybackFrame(int idx)
{
    if (idx < 0 || idx >= m_playbackKeys.size())
        return;

    QByteArray data = m_playbackFrames.value(m_playbackKeys[idx]);
    if (data.isEmpty())
        return;

    int beamCount = config.getBeamCounts();
    // 不传 Replay 参数，走编码器计算路径，帧位置与实时采集一致
    ui->View_1->slot_Recive_date(beamCount, data);
    ui->View_2->slot_Recive_date(beamCount, data);
    ui->View_3->slot_Recive_date(beamCount, data);
    ui->View_4->slot_Recive_date(beamCount, data);
    ui->measure_widget->slot_Recive_date(data);
}

void MainWindow::on_BtnAdd_clicked()
{
    // 弹出添加设备对话框
    AddDeviceDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return; // 用户取消了
    }

    QString ip = dialog.getIpAddress();
    int newServerId = dialog.getDeviceId();

    // 检查IP+ID组合是否已存在
    DeviceKey newDevice { ip, newServerId };

    // 检查是否已存在（从config中检查）
    auto servers = config.getAllServers();
    bool exists = false;
    for (const auto &server : servers) {
        if (QString::fromStdString(server.address) == ip && server.serverId == newServerId) {
            exists = true;
            break;
        }
    }

    if (exists) {
        QMessageBox::warning(this, tr("Warning"),
                             tr("Device %1 already exists!").arg(newDevice.toString()));
        return;
    }

    // 检查IP是否已存在（可选：警告但允许添加不同ID）
    bool ipExists = false;
    for (const auto &server : servers) {
        if (QString::fromStdString(server.address) == ip) {
            ipExists = true;
            break;
        }
    }

    if (ipExists) {
        // IP已存在但ID不同，询问是否继续
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, tr("Confirm"),
            tr("IP %1 already exists with different Device ID.\nDo you want to add this device?")
                .arg(ip),
            QMessageBox::Yes | QMessageBox::No);

        if (reply != QMessageBox::Yes) {
            return;
        }
    }

    // 添加设备
    config.addServer(ip.toStdString(), newServerId);
    m_deviceStates[newDevice] = ConState::Unknown;

    // 刷新下拉框
    refreshDeviceComboBox();

    // 选中新添加的设备
    int index = ui->comboBox_ip->findText(ip);
    if (index >= 0) {
        ui->comboBox_ip->setCurrentIndex(index);
        int idIndex = ui->comboBox_id->findText(QString::number(newServerId));
        if (idIndex >= 0) {
            ui->comboBox_id->setCurrentIndex(idIndex);
        }
    }

    QMessageBox::information(this, tr("Success"),
                             tr("Device %1 added successfully!").arg(newDevice.toString()));
}

void MainWindow::on_BtnRemove_clicked()
{
    DeviceKey currentDevice = getCurrentDeviceKey();

    if (currentDevice.ip.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No device selected!"));
        return;
    }

    // 检查是否只有一个设备，至少保留一个
    auto servers = config.getAllServers();
    if (servers.size() <= 1) {
        QMessageBox::warning(this, tr("Warning"), tr("At least one device must be retained!"));
        return;
    }

    if (m_deviceStates.value(currentDevice) == ConState::Connected) {
        DisconnectDevice(currentDevice);
    }

    // 从Client移除
    config.removeServer(currentDevice.ip.toStdString(), currentDevice.deviceId);

    // 从状态Map移除
    m_deviceStates.remove(currentDevice);

    // 刷新下拉框
    refreshDeviceComboBox();

    QMessageBox::information(this, tr("Success"),
                             tr("Device %1 removed successfully!").arg(currentDevice.toString()));
}
// IP地址格式验证函数
bool MainWindow::isValidIpAddress(const QString &ip)
{
    // 正则表达式验证IPv4地址
    QRegularExpression ipRegex(
        "^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");

    if (!ipRegex.match(ip).hasMatch()) {
        return false;
    }

    // 可选：特殊处理localhost
    if (ip == "127.0.0.1" || ip == "localhost") {
        return true;
    }

    return true;
}
