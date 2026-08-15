#include "parammanager.h"
#include "qcoreapplication.h"

ParamManager *ParamManager::instance = nullptr;

ParamManager::ParamManager(QObject *parent)
    : QObject(parent)
{
}

ParamManager::~ParamManager() { }

ParamManager *ParamManager::getInstance()
{
    if (!instance) {
        instance = new ParamManager();
    }
    return instance;
}

QString ParamManager::getParamFilePath(const QString &fileName)
{
    // 在应用程序目录下创建params文件夹
    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);
    if (!dir.exists("params")) {
        dir.mkdir("params");
    }
    return appDir + "/params/" + fileName;
}

bool ParamManager::saveParams(const QJsonObject &params, const QString &fileName)
{
    QString filePath = getParamFilePath(fileName);
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QJsonDocument doc(params);
    file.write(doc.toJson());
    file.close();

    return true;
}

QJsonObject ParamManager::loadParams(const QString &fileName)
{
    QString filePath = getParamFilePath(fileName);
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        return QJsonObject();
    }

    return doc.object();
}

bool ParamManager::paramFileExists(const QString &fileName)
{
    QString filePath = getParamFilePath(fileName);
    return QFile::exists(filePath);
}
