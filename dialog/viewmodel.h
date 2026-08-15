#ifndef VIEWMODEL_H
#define VIEWMODEL_H

#include "dialog/viewwidget.h"
#include <QWidget>
#include <QtConcurrent/QtConcurrent>
#include <libkuka3d.h>
#include <deque>

class ViewModel : public QWidget
{
    Q_OBJECT

public:
    explicit ViewModel(QWidget *parent = nullptr);

    ~ViewModel();

    void slot_Recive_date(int beam, const QByteArray &datapacket, bool Replay = false, int No = 0);

private:
    void init3DView();

    void extractAndPushDefectCloudData(int beam_0, const QByteArray &datapacket);

    void processDataAsync(int beam, const QByteArray &datapacket);

    bool m_is3DInitialized = false;
    int m_frameCount = 0;
    bool m_smoothValid[64] = { false };
    int m_invalidRun[64] = { 0 };
    std::deque<int> m_firstHist;
    std::deque<int> m_lastHist;
    QTimer* m_robotTimer = nullptr;
    QTimer* m_robotTimer1 = nullptr;

    Kuka3D::LibKuka3D *m_libKuka3D = nullptr;

signals:
    void ultrasoundDataReady(const QByteArray& datagram, int deviceId);


};

#endif // VIEWMODEL_H
