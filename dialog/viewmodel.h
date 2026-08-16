#ifndef VIEWMODEL_H
#define VIEWMODEL_H

#include <QByteArray>
#include <QWidget>
#include <deque>

class ViewModel : public QWidget
{
    Q_OBJECT

public:
    explicit ViewModel(QWidget *parent = nullptr);

    ~ViewModel();

    void slot_Recive_date(int beam, const QByteArray &datapacket, bool Replay = false, int No = 0);

private:
    void extractAndPushDefectCloudData(int beam_0, const QByteArray &datapacket);
};

#endif // VIEWMODEL_H
