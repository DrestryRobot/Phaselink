#ifndef PARAMMANAGER_H
#define PARAMMANAGER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QDebug>

class ParamManager : public QObject
{
    Q_OBJECT

public:
    static ParamManager *getInstance();

    // 保存参数到文件
    bool saveParams(const QJsonObject &params, const QString &fileName = "scan_params.json");

    // 从文件加载参数
    QJsonObject loadParams(const QString &fileName = "scan_params.json");

    // 检查文件是否存在
    bool paramFileExists(const QString &fileName = "scan_params.json");

private:
    explicit ParamManager(QObject *parent = nullptr);
    ~ParamManager();

    static ParamManager *instance;
    QString getParamFilePath(const QString &fileName);
};

#endif // PARAMMANAGER_H
