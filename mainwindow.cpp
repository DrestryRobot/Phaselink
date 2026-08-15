#include "mainwindow.h"
#include "datadispatch.h"

#include <app.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QListView>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSet>
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
    DataDispatch::removeProcessor(m_dataProcessor);
    config.startCapture(false);
    for (auto it = m_deviceStates.begin(); it != m_deviceStates.end(); ++it) {
        if (it.value() == ConState::Connected) {
            config.Disconnect();
            break;
        }
    }

    for (int i = 0; i < 50; i++)
        QThread::msleep(2);

    if (m_playbackTimer)
        m_playbackTimer->stop();

    if (m_dataProcessor) {
        m_dataProcessor->stop();
        for (int i = 0; i < 100 && m_dataProcessor->isProcessing(); i++)
            QThread::msleep(1);
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
    for (int i = 0; i < ui->Top_funct_btnGroup->buttons().size(); i++)
        ui->Top_funct_btnGroup->setId(ui->Top_funct_btnGroup->buttons().at(i), i);

    m_processorThread = new QThread(this);
    m_processorThread->setObjectName(QStringLiteral("DataProcessorThread"));
    m_dataProcessor = new DataProcessor();
    m_dataProcessor->moveToThread(m_processorThread);
    g_dataProcessor = m_dataProcessor;
    m_processorThread->start();

    refreshViewSize();

    ui->View_1->setScanView(ViewWidget::A_Scan);
    ui->View_2->setScanView(ViewWidget::E_Scan);
    ui->View_2->setColorPalette(ui->S_Scan_Color->getColors());

    ui->View_3->setScanView(ViewWidget::C1_Scan);
    ui->View_3->setColorPalette(ui->Amp_color->getColors());
    ui->View_3->dragBarShow(false);
    ui->C_Scan->hide();
    App::getInstance()->EncoderTimeMode = 0;

    ui->ruler_range->setDirections(Directions::Vertical_left);
    ui->ruler_range->setPosStart(0.0);
    ui->ruler_range->setPosEnd(50.0);

    double end = (config.getBeamCounts() - 1) * 0.6 * 1;
    ui->ruler_distance_s->setPosStart(0);
    ui->ruler_distance_s->setPosEnd(end);

    ui->ruler_distance_c1->setDirections(Directions::Vertical_left);
    ui->ruler_distance_c1->setPosStart(0);
    ui->ruler_distance_c1->setPosEnd(end);

    ui->ruler_widget->setDirections(Directions::Horizontal);
    if (App::getInstance()->EncoderIndex == 0) {
        ui->ruler_widget->setRulerUnit("mm");
        ui->ruler_widget->setPosStart(App::getInstance()->ScanStart);
        ui->ruler_widget->setPosEnd(App::getInstance()->ScanEnd);
    } else {
        ui->ruler_widget->setRulerUnit("s");
        ui->ruler_widget->setPosStart(0);
        ui->ruler_widget->setPosEnd(App::getInstance()->Scanlength);
    }

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
        refreshDeviceComboBox();
    });

    m_dataSaver = new PacketDataSaver(this);
    m_playbackTimer = new QTimer(this);

    connect(m_playbackTimer, &QTimer::timeout, this, [this]() {
        if (m_playbackKeys.isEmpty())
            return;
        m_currentPlaybackIdx++;
        if (m_currentPlaybackIdx >= m_playbackKeys.size()) {
            m_playbackTimer->stop();
            m_currentPlaybackIdx = m_playbackKeys.size() - 1;
            return;
        }
        ui->spinFrame->blockSignals(true);
        ui->spinFrame->setValue(m_playbackKeys[m_currentPlaybackIdx]);
        ui->spinFrame->blockSignals(false);
        sendPlaybackFrame(m_currentPlaybackIdx);
        ui->View_3->setFrameLabel(m_playbackKeys[m_currentPlaybackIdx]);
    });

    connect(App::getInstance(), &App::signal_frameChanged, this, [this](int) {
        if (!App::getInstance()->Replay || m_playbackKeys.isEmpty())
            return;
    });
}

void MainWindow::initSlot()
{
    connect(ui->Top_funct_btnGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            this, &MainWindow::slot_pushButton_click);

    config.setDataPacketCallback(DataDispatch::dataPacketCallback);
    QMetaObject::invokeMethod(m_dataProcessor, [=]() {
        DataDispatch::addProcessor(m_dataProcessor);
        m_dataProcessor->setViews(ui->View_1, ui->View_2, nullptr, nullptr, ui->measure_widget);
    }, Qt::QueuedConnection);

    connect(App::getInstance(), &App::signal_Reset_CScan, this, [=](int index) {
        App::getInstance()->EncoderTimeMode = index;
        ui->View_3->initCsacnView();
        ui->View_3->Isstart = true;
        ui->View_3->dragBarShow(false);
        ui->View_3->resetDragBar();
        ui->ruler_widget->setDirections(Directions::Horizontal);
        if (index == 0) {
            ui->ruler_widget->setRulerUnit("mm");
            ui->ruler_widget->setPosStart(App::getInstance()->ScanStart);
            ui->ruler_widget->setPosEnd(App::getInstance()->ScanEnd);
        } else {
            ui->ruler_widget->setRulerUnit("s");
            ui->ruler_widget->setPosStart(0);
            ui->ruler_widget->setPosEnd(App::getInstance()->Scanlength);
        }
        ui->SaveData->setEnabled(true);
    });

    connect(App::getInstance(), &App::signal_soundChanged, this, [=]() {
    });

    connect(App::getInstance(), &App::signal_RangeChanged, this,
            &MainWindow::slot_rulerWidgetChanged);

    connect(App::getInstance(), &App::signal_probeChange, this,
            &MainWindow::slot_rulerProbeChanged);

    connect(App::getInstance(), &App::signal_paletteChanged, this, [=] {
        ui->View_2->setColorPalette(ui->S_Scan_Color->getColors());
        ui->View_3->setColorPalette(ui->Amp_color->getColors());
    });

    connect(ui->disconnect_btn, &QPushButton::clicked, this, &MainWindow::DisconnectClick);

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
    QString currentIp = ui->comboBox_ip->currentText();
    int currentId = ui->comboBox_id->currentText().toInt();

    auto servers = config.getAllServers();

    QSet<QString> uniqueIps;
    for (const auto &server : servers)
        uniqueIps.insert(QString::fromStdString(server.address));

    QSet<QString> existingIps;
    for (int i = 0; i < ui->comboBox_ip->count(); ++i)
        existingIps.insert(ui->comboBox_ip->itemText(i));

    for (const QString &ip : uniqueIps) {
        if (!existingIps.contains(ip))
            ui->comboBox_ip->addItem(ip);
    }
    for (int i = ui->comboBox_ip->count() - 1; i >= 0; --i) {
        QString ip = ui->comboBox_ip->itemText(i);
        if (!uniqueIps.contains(ip))
            ui->comboBox_ip->removeItem(i);
    }

    int ipIndex = ui->comboBox_ip->findText(currentIp);
    if (ipIndex >= 0)
        ui->comboBox_ip->setCurrentIndex(ipIndex);
    else if (ui->comboBox_ip->count() > 0)
        ui->comboBox_ip->setCurrentIndex(0);

    updateDeviceComboBoxForIp(ui->comboBox_ip->currentText());

    int idIndex = ui->comboBox_id->findText(QString::number(currentId));
    if (idIndex >= 0)
        ui->comboBox_id->setCurrentIndex(idIndex);
    else if (ui->comboBox_id->count() > 0)
        ui->comboBox_id->setCurrentIndex(0);

    m_currentSelectedDevice = getCurrentDeviceKey();
    updateConnectButtonState();
}

void MainWindow::updateDeviceComboBoxForIp(const QString &ip)
{
    QSet<int> validIds;
    auto servers = config.getAllServers();
    for (const auto &server : servers) {
        if (QString::fromStdString(server.address) == ip)
            validIds.insert(server.serverId);
    }

    QSet<int> existingIds;
    for (int i = 0; i < ui->comboBox_id->count(); ++i)
        existingIds.insert(ui->comboBox_id->itemText(i).toInt());

    for (int id : validIds) {
        if (!existingIds.contains(id))
            ui->comboBox_id->addItem(QString::number(id));
    }
    for (int i = ui->comboBox_id->count() - 1; i >= 0; --i) {
        int id = ui->comboBox_id->itemText(i).toInt();
        if (!validIds.contains(id))
            ui->comboBox_id->removeItem(i);
    }

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

    if (currentState == ConState::Failed)
        m_deviceStates[currentDevice] = ConState::Unconnected;

    auto ser = config.getAllServers();
    for (auto server : ser) {
        if (server.address == currentDevice.ip.toStdString()
            && server.serverId == currentDevice.deviceId) {
            config.setCurrentServerId(server.serverId);
            break;
        }
    }

    // 清空回放状态，切换到在线模式
    App::getInstance()->Replay = false;
    m_playbackTimer->stop();
    m_playbackFrames.clear();
    m_playbackKeys.clear();
    m_currentPlaybackIdx = 0;
    ui->View_3->dragBarShow(false);
    ui->View_3->resetDragBar();
    ui->View_3->Isstart = true;
    {
        QMutexLocker locker(&App::getInstance()->recordBufferMutex);
        App::getInstance()->recordBuffer.clear();
    }
    m_dataProcessor->resetRecording();
    m_dataProcessor->stop();
    disconnect(ui->View_3, &ViewWidget::playbackFrameChanged, this, nullptr);
    disconnect(ui->spinFrame, QOverload<int>::of(&QSpinBox::valueChanged), this, nullptr);
    ui->View_3->initCsacnView();

    bool flag = config.Connect();

    if (flag) {
        config.startCapture(true);
        m_deviceStates[currentDevice] = ConState::Connected;
        updateConnectButtonState();

        QTimer::singleShot(500, this, [] {
            emit App::getInstance()->refresh_Allpara();
            emit App::getInstance()->signal_GateView_Refresh();
            emit App::getInstance()->signal_RangeChanged();
            emit App::getInstance()->signal_Connected();
            emit App::getInstance()->signal_tcgChanged();
        });

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
        if (m_currentSelectedDevice == device)
            updateConnectButtonState();
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
    if (!currentDevice.ip.isEmpty())
        DisconnectDevice(currentDevice);
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
        if (IsConnect == ConState::Unconnected)
            ui->connectBtn->setText(tr("Unconnected"));
        else if (IsConnect == ConState::Connected)
            ui->connectBtn->setText(tr("Connecting"));
        else
            ui->connectBtn->setText(tr("Failed"));
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
    if (Num > 1)
        ui->add_sud_widget->hide();
    else
        ui->add_sud_widget->show();
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
    ui->startscan_btn->setEnabled(true);
    ui->connect_btn->setCheckable(true);
    ui->Replay->setEnabled(false);
    App::getInstance()->Replay = false;
    {
        QMutexLocker locker(&App::getInstance()->recordBufferMutex);
        App::getInstance()->recordBuffer.clear();
    }
    m_dataProcessor->resetRecording();
    m_playbackFrames.clear();
    m_playbackKeys.clear();
    QTimer::singleShot(100, this, [=]() {
        ui->View_3->initCsacnView();
        ui->View_3->Isstart = true;
    });
}

void MainWindow::refreshViewSize()
{
    int w = qMax(1, ui->view_widget->width());
    int h = qMax(1, this->height() - 160);
    ui->View_1->setMaximumWidth(w / 2);
    ui->S_Scan->setMaximumWidth(w / 2);
    if (ui->viewComboBox->currentIndex() == 0) {
        ui->View_1->setMaximumHeight(h);
        ui->S_Scan->setMaximumHeight(h);
        ui->C_Scan->hide();
    } else {
        ui->View_1->setMaximumHeight(h / 2);
        ui->S_Scan->setMaximumHeight(h / 2);
        ui->C_Scan->setMaximumHeight(h / 2);
        ui->C_Scan->show();
    }
    ui->View_1->onViewResized();
    ui->View_2->onViewResized();
    ui->View_3->onViewResized();
}

void MainWindow::on_comboBox_currentIndexChanged(int index)
{
    if (index)
        lang.load(QCoreApplication::applicationDirPath() + "/phaselink_EN.qm");
    else
        lang.load(QCoreApplication::applicationDirPath() + "/phaselink_CN.qm");
    qApp->installTranslator(&lang);
}

void MainWindow::on_comboBox_ip_currentIndexChanged(int index)
{
    Q_UNUSED(index);

    QString currentIp = ui->comboBox_ip->currentText();
    updateDeviceComboBoxForIp(currentIp);

    m_currentSelectedDevice = getCurrentDeviceKey();
    updateConnectButtonState();

    if (!m_currentSelectedDevice.ip.isEmpty()) {
        auto servers = config.getAllServers();
        for (const auto &server : servers) {
            if (server.serverId == m_currentSelectedDevice.deviceId) {
                config.setCurrentServerId(server.serverId);
                break;
            }
        }
    }

    if (!m_currentSelectedDevice.ip.isEmpty()
        && m_deviceStates.value(m_currentSelectedDevice) == ConState::Connected) {
        emit App::getInstance()->refresh_Allpara();
        emit App::getInstance()->signal_GateView_Refresh();
        emit App::getInstance()->signal_RangeChanged();
        emit App::getInstance()->signal_Connected();
    }
}

void MainWindow::on_comboBox_id_currentIndexChanged(int index)
{
    Q_UNUSED(index);

    m_currentSelectedDevice = getCurrentDeviceKey();
    updateConnectButtonState();

    if (!m_currentSelectedDevice.ip.isEmpty()) {
        auto servers = config.getAllServers();
        for (const auto &server : servers) {
            if (server.serverId == m_currentSelectedDevice.deviceId) {
                config.setCurrentServerId(server.serverId);
                break;
            }
        }
    }

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
    if (index == 1) {
        if (!m_cScanViewInitialized) {
            ui->View_3->initCsacnView();
            ui->View_3->Isstart = true;
            m_cScanViewInitialized = true;
        }
        QMetaObject::invokeMethod(m_dataProcessor, [=]() {
            m_dataProcessor->setViews(ui->View_1, ui->View_2, ui->View_3, nullptr, ui->measure_widget);
        }, Qt::QueuedConnection);
        ui->View_3->onViewResized();
    }
}

void MainWindow::on_lineEdit_editingFinished()
{
    QString ip = ui->lineEdit->text().trimmed();
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
    ui->ruler_range->setPosStart(config.getRangeStart());
    ui->ruler_range->setPosEnd(config.getRangeEnd());

    ui->ruler_widget->setDirections(Directions::Horizontal);
    if (App::getInstance()->EncoderIndex == 0) {
        ui->ruler_widget->setRulerUnit("mm");
        ui->ruler_widget->setPosStart(App::getInstance()->ScanStart);
        ui->ruler_widget->setPosEnd(App::getInstance()->ScanEnd);
    } else {
        ui->ruler_widget->setRulerUnit("s");
        ui->ruler_widget->setPosStart(0);
        ui->ruler_widget->setPosEnd(App::getInstance()->Scanlength);
    }
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
    ui->ruler_distance_c1->setPosStart(0);
    ui->ruler_distance_c1->setPosEnd(end);
}

void MainWindow::on_SaveData_clicked()
{
    if (!m_dataProcessor->isSavingEnabled()) {
        {
            QMutexLocker locker(&App::getInstance()->recordBufferMutex);
            App::getInstance()->recordBuffer.clear();
        }
        m_dataProcessor->resetRecording();
        m_dataProcessor->setSavingEnabled(true);
        ui->SaveData->setText(tr("Save to File"));
        return;
    }

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
    QDir dataDir(QCoreApplication::applicationDirPath() + "/data");
    if (!dataDir.exists())
        dataDir.mkpath(".");
    QString fileName = dataDir.filePath(
        QString("data_%1.pdata").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")));

    m_dataSaver->setData(bufferSnapshot);
    int beamCount = config.getBeamCounts();
    int points = config.getPointQuantity();
    if (m_dataSaver->saveToFile(fileName, beamCount, points)) {
        QMessageBox::information(this, tr("Info"), tr("Recording saved to:\n%1").arg(fileName));
        ui->Replay->setEnabled(true);
    } else {
        QMessageBox::warning(this, tr("Warning"), tr("Failed to save recording!"));
    }
    {
        QMutexLocker locker(&App::getInstance()->recordBufferMutex);
        App::getInstance()->recordBuffer.clear();
    }
}

void MainWindow::on_Replay_clicked()
{
    for (auto it = m_deviceStates.begin(); it != m_deviceStates.end(); ++it) {
        if (it.value() == ConState::Connected) {
            config.Disconnect();
            it.value() = ConState::Unconnected;
        }
    }
    updateConnectButtonState();
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
    m_playbackBeamCount = config.getBeamCounts();

    if (m_playbackKeys.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No frame data found in file!"));
        m_dataProcessor->start();
        return;
    }

    App::getInstance()->Replay = true;

    {
        int mode = m_dataSaver->loadedEncoderTimeMode();
        App::getInstance()->EncoderTimeMode = mode;
        ui->ruler_widget->setDirections(Directions::Horizontal);
        if (mode == 0) {
            ui->ruler_widget->setRulerUnit("mm");
            ui->ruler_widget->setPosStart(App::getInstance()->ScanStart);
            ui->ruler_widget->setPosEnd(App::getInstance()->ScanEnd);
        } else {
            ui->ruler_widget->setRulerUnit("s");
            ui->ruler_widget->setPosStart(0);
            ui->ruler_widget->setPosEnd(App::getInstance()->Scanlength);
        }
    }

    emit App::getInstance()->refresh_Allpara();
    emit App::getInstance()->signal_GateView_Refresh();
    emit App::getInstance()->signal_RangeChanged();
    emit App::getInstance()->signal_Connected();
    emit App::getInstance()->signal_tcgChanged();

    emit App::getInstance()->signal_Reset_CScan(m_dataSaver->loadedEncoderTimeMode());

    // 先初始化C扫缓冲区，再渲染完整C扫图
    ui->View_3->initCsacnView();
    ui->View_3->Isstart = true;
    {
        int bc = m_playbackBeamCount > 0 ? m_playbackBeamCount : config.getBeamCounts();
        for (int i = 0; i < m_playbackKeys.size(); i++) {
            QByteArray fd = m_playbackFrames.value(m_playbackKeys[i]);
            if (!fd.isEmpty())
                ui->View_3->slot_Recive_date(bc, fd);
        }
    }

    ui->spinFrame->setRange(m_playbackKeys.first(), m_playbackKeys.last());
    ui->spinFrame->setValue(m_playbackKeys.first());
    disconnect(ui->spinFrame, QOverload<int>::of(&QSpinBox::valueChanged), this, nullptr);
    connect(ui->spinFrame, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int val) {
        int idx = findClosestKey(val);
        m_currentPlaybackIdx = idx;
        sendPlaybackFrame(idx);
        ui->View_3->setFrameLabel(m_playbackKeys[idx]);
    });

    if (m_dataSaver->loadedEncoderTimeMode() == 1) {
        ui->View_3->setPlaybackSliderRange(m_playbackKeys.size() - 1);
        ui->View_3->resetDragBar();
        ui->View_3->dragBarShow(true);
        disconnect(ui->View_3, &ViewWidget::playbackFrameChanged, this, nullptr);
        connect(ui->View_3, &ViewWidget::playbackFrameChanged, this,
                [this](int col) {
            int idx = findClosestKey(col);
            m_currentPlaybackIdx = idx;
            ui->spinFrame->blockSignals(true);
            ui->spinFrame->setValue(m_playbackKeys[idx]);
            ui->spinFrame->blockSignals(false);
            sendPlaybackFrame(idx);
            ui->View_3->setFrameLabel(m_playbackKeys[idx]);
        });
    }

    m_currentPlaybackIdx = 0;
    sendPlaybackFrame(0);
    ui->View_3->setFrameLabel(m_playbackKeys[0]);
    m_playbackTimer->start();
}

void MainWindow::sendPlaybackFrame(int idx)
{
    if (idx < 0 || idx >= m_playbackKeys.size())
        return;

    QByteArray data = m_playbackFrames.value(m_playbackKeys[idx]);
    if (data.isEmpty())
        return;

    int bc = m_playbackBeamCount > 0 ? m_playbackBeamCount : config.getBeamCounts();
    ui->View_1->slot_Recive_date(bc, data);
    ui->View_2->slot_Recive_date(bc, data);
    ui->View_3->slot_Recive_date(bc, data);
    ui->measure_widget->slot_Recive_date(data);
}

void MainWindow::on_BtnAdd_clicked()
{
    AddDeviceDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    QString ip = dialog.getIpAddress();
    int newServerId = dialog.getDeviceId();
    DeviceKey newDevice { ip, newServerId };

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

    bool ipExists = false;
    for (const auto &server : servers) {
        if (QString::fromStdString(server.address) == ip) {
            ipExists = true;
            break;
        }
    }

    if (ipExists) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, tr("Confirm"),
            tr("IP %1 already exists with different Device ID.\nDo you want to add this device?")
                .arg(ip),
            QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes)
            return;
    }

    config.addServer(ip.toStdString(), newServerId);
    m_deviceStates[newDevice] = ConState::Unknown;
    refreshDeviceComboBox();

    int index = ui->comboBox_ip->findText(ip);
    if (index >= 0) {
        ui->comboBox_ip->setCurrentIndex(index);
        int idIndex = ui->comboBox_id->findText(QString::number(newServerId));
        if (idIndex >= 0)
            ui->comboBox_id->setCurrentIndex(idIndex);
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

    auto servers = config.getAllServers();
    if (servers.size() <= 1) {
        QMessageBox::warning(this, tr("Warning"), tr("At least one device must be retained!"));
        return;
    }

    if (m_deviceStates.value(currentDevice) == ConState::Connected)
        DisconnectDevice(currentDevice);

    config.removeServer(currentDevice.ip.toStdString(), currentDevice.deviceId);
    m_deviceStates.remove(currentDevice);
    refreshDeviceComboBox();

    QMessageBox::information(this, tr("Success"),
                             tr("Device %1 removed successfully!").arg(currentDevice.toString()));
}

int MainWindow::findClosestKey(quint32 key) const
{
    auto it = std::lower_bound(m_playbackKeys.begin(), m_playbackKeys.end(), key);
    if (it == m_playbackKeys.end())
        return m_playbackKeys.size() - 1;
    if (it == m_playbackKeys.begin())
        return 0;
    int idx = std::distance(m_playbackKeys.begin(), it);
    quint32 diffPrev = key - m_playbackKeys[idx - 1];
    quint32 diffNext = m_playbackKeys[idx] - key;
    return (diffPrev < diffNext) ? idx - 1 : idx;
}

bool MainWindow::isValidIpAddress(const QString &ip)
{
    QRegularExpression ipRegex(
        "^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    if (!ipRegex.match(ip).hasMatch())
        return false;
    if (ip == "127.0.0.1" || ip == "localhost")
        return true;
    return true;
}
