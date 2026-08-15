#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <client.h>

#include <QAbstractButton>
#include <QMainWindow>
#include <QTimer>
#include <QTranslator>
#include <QPair>

#include "dialog/dataprocessor.h"
#include "dialog/viewwidget.h"
#include "dialog/PacketDataSaver.h"
#include "qthread.h"

enum class ConState { Unknown, Connected, Unconnected, Failed };

struct DeviceKey
{
    QString ip;
    int deviceId;

    bool operator==(const DeviceKey &other) const
    {
        return ip == other.ip && deviceId == other.deviceId;
    }

    bool operator<(const DeviceKey &other) const
    {
        if (ip != other.ip)
            return ip < other.ip;
        return deviceId < other.deviceId;
    }

    QString toString() const { return QString("%1:%2").arg(ip).arg(deviceId); }
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool isValidIpAddress(const QString &ip);
    void updateConnectButtonState();
    void DisconnectDevice(const DeviceKey &device);

public slots:
    void on_BtnRemove_clicked();

private slots:
    void slot_pushButton_click(QAbstractButton *btn);
    void on_connect_btn_clicked();
    void on_startscan_btn_clicked();
    void on_stopscan_btn_clicked();
    void on_Origin_btn_clicked();
    void DisconnectClick();
    void refreshViewSize();

    void on_comboBox_currentIndexChanged(int index);
    void on_comboBox_ip_currentIndexChanged(int index);
    void on_comboBox_id_currentIndexChanged(int index);
    void on_viewComboBox_currentIndexChanged(int index);
    void on_lineEdit_editingFinished();

    void slot_rulerWidgetChanged();
    void slot_rulerProbeChanged();

    void on_SaveData_clicked();
    void on_Replay_clicked();
    void on_BtnAdd_clicked();

private:
    void initWidget();
    void initSlot();
    void resizeEvent(QResizeEvent *e) override;
    virtual void changeEvent(QEvent *event) Q_DECL_OVERRIDE;
    virtual void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;

    void refreshDeviceComboBox();
    void sendPlaybackFrame(int idx);
    DeviceKey getCurrentDeviceKey();
    void updateDeviceComboBoxForIp(const QString &ip);

private:
    Ui::MainWindow *ui;
    Client &config;
    ConState IsConnect = ConState::Unconnected;
    QTranslator lang;
    QVector<int> AmpData;
    std::string ipAddress;
    QByteArray curDatapacket;
    QMap<quint32, QByteArray> m_playbackFrames;
    QList<quint32> m_playbackKeys;
    PacketDataSaver *m_dataSaver = nullptr;
    int m_playbackBeamCount = 0;
    DataProcessor *m_dataProcessor = nullptr;
    QThread *m_processorThread = nullptr;
    QTimer *m_playbackTimer = nullptr;
    int m_currentPlaybackIdx = 0;
    int findClosestKey(quint32 key) const;
    bool m_cScanViewInitialized = false;
    bool m_isASC = false;

    QMap<DeviceKey, ConState> m_deviceStates;
    DeviceKey m_currentSelectedDevice;
};

inline uint qHash(const DeviceKey &key, uint seed)
{
    return qHash(key.ip, seed) ^ (key.deviceId + seed);
}

#endif // MAINWINDOW_H
