#include <QStyle>
#include <QFile>
#include <QEvent>
#include <QDebug>
#include "ndtbase.h"
#include "qregularexpression.h"

class NdtBase::_NdtBaseData
{
public:
    QString styleFilePath = "";

    QString styleString(QString filePath, NdtBase::ShowModel model = ShowModel::NightTime);
};

NdtBase::NdtBase(QWidget *parent)
    : QWidget(parent)
    , selfData(new NdtBase::_NdtBaseData)
{
}

NdtBase::~NdtBase()
{
    delete selfData;
}

void NdtBase::setStyleFilePath(const QString &path) const
{
    selfData->styleFilePath = path;
}

void NdtBase::setShowModel(ShowModel model)
{
    style()->unpolish(this);
    this->setStyleSheet("");
    this->setStyleSheet(
        QString::fromUtf8(selfData->styleString(selfData->styleFilePath, model).toUtf8()));
    style()->polish(this);
}

void NdtBase::setFunction1(QString, QString) { }

void NdtBase::refreshValue() { }

bool NdtBase::itemVisible()
{
    return !(this->visibleRegion().isNull());
}

QString NdtBase::_NdtBaseData::styleString(QString filePath, ShowModel model)
{
    QStringList styleStr;
    QFile file(filePath);


    QString styleModel = (model == ShowModel::DayTime) ? "/*am*/" : "/*pm*/";

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return "";
    }

    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        QString lineStr(line.data());
        lineStr.remove(QRegularExpression("[\t\r\n]"));
        if (lineStr.contains("/*")) {
            if (lineStr.contains(styleModel)) {
                lineStr.remove(styleModel);
                styleStr << lineStr;
            }
            continue;
        }
        if (lineStr.isEmpty())
            continue;
        styleStr << lineStr;
    }

    file.close();
    return styleStr.join("\n");
}
