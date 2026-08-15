#ifndef DUALDEVICESELECTDIALOG_H
#define DUALDEVICESELECTDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMap>
#include <QVector>

#include "mainwindow.h"

class DualDeviceSelectDialog : public QDialog
{
public:
    DualDeviceSelectDialog(const QMap<DeviceKey, ConState> &deviceStates, QWidget *parent = nullptr)
        : QDialog(parent)
        , m_deviceStates(deviceStates)
    {
        setWindowTitle(tr("Select Dual Devices"));
        setModal(true);
        setFixedSize(350, 200);

        // 创建控件
        QLabel *device1Label = new QLabel(tr("Device 1:"));
        QLabel *device2Label = new QLabel(tr("Device 2:"));

        device1Combo = new QComboBox;
        device2Combo = new QComboBox;

        QPushButton *okButton = new QPushButton(tr("OK"));
        QPushButton *cancelButton = new QPushButton(tr("Cancel"));

        // 填充设备列表（只显示已连接的设备）
        populateDeviceList();

        // 布局
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        QHBoxLayout *device1Layout = new QHBoxLayout;
        QHBoxLayout *device2Layout = new QHBoxLayout;
        QHBoxLayout *buttonLayout = new QHBoxLayout;

        device1Layout->addWidget(device1Label);
        device1Layout->addWidget(device1Combo);
        device2Layout->addWidget(device2Label);
        device2Layout->addWidget(device2Combo);
        buttonLayout->addWidget(okButton);
        buttonLayout->addWidget(cancelButton);

        mainLayout->addLayout(device1Layout);
        mainLayout->addLayout(device2Layout);
        mainLayout->addSpacing(20);
        mainLayout->addLayout(buttonLayout);

        // 连接信号 - 使用lambda替代槽函数
        connect(okButton, &QPushButton::clicked, this, [this]() {
            if (device1Combo->currentIndex() < 0 || device2Combo->currentIndex() < 0) {
                QMessageBox::warning(this, tr("Warning"), tr("Please select both devices!"));
                return;
            }

            if (device1Combo->currentIndex() == device2Combo->currentIndex()) {
                QMessageBox::warning(this, tr("Warning"), tr("Please select two different devices!"));
                return;
            }

            accept();
        });
        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    }

    DeviceKey getDevice1() const
    {
        if (device1Combo->currentIndex() < 0)
            return DeviceKey();
        return m_devices[device1Combo->currentIndex()];
    }

    DeviceKey getDevice2() const
    {
        if (device2Combo->currentIndex() < 0)
            return DeviceKey();
        return m_devices[device2Combo->currentIndex()];
    }

private:
    void populateDeviceList()
    {
        m_devices.clear();
        device1Combo->clear();
        device2Combo->clear();

        for (auto it = m_deviceStates.begin(); it != m_deviceStates.end(); ++it) {
            // 只添加已连接的设备
            if (it.value() == ConState::Connected) {
                m_devices.append(it.key());
                QString displayText = QString("%1 (ID:%2)").arg(it.key().ip).arg(it.key().deviceId);
                device1Combo->addItem(displayText);
                device2Combo->addItem(displayText);
            }
        }

        // 默认选择第一个和第二个设备
        if (device1Combo->count() > 0)
            device1Combo->setCurrentIndex(0);
        if (device2Combo->count() > 1)
            device2Combo->setCurrentIndex(1);
    }

private:
    const QMap<DeviceKey, ConState> &m_deviceStates;
    QVector<DeviceKey> m_devices;
    QComboBox *device1Combo;
    QComboBox *device2Combo;
};

#endif // DUALDEVICESELECTDIALOG_H
