#ifndef NDTBASE_H
#define NDTBASE_H

#include <QWidget>

// 所有自定义类的基类
// 目前的功能:
// 1 改变显示模式(白天、黑夜)

class NdtBase : public QWidget
{
    Q_OBJECT

public:
    enum ShowModel { DayTime, NightTime };

    explicit NdtBase(QWidget *parent = nullptr);
    ~NdtBase();

    void setStyleFilePath(const QString &) const;

    virtual void setShowModel(NdtBase::ShowModel model);

    virtual void setFunction1(QString setf, QString getf);
    virtual void refreshValue();

    bool itemVisible();

private:
    class _NdtBaseData;
    _NdtBaseData *selfData;
};

#endif // NDTBASE_H
