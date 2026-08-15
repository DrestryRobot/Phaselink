#include "datadispatch.h"
#include "client.h"
#include "dialog/dataprocessor.h"
#include <QMetaObject>

QList<DataProcessor*> DataDispatch::s_processors;
QList<DataDispatch::SinkPtr> DataDispatch::s_directSinks;
QMutex DataDispatch::s_mutex;

void DataDispatch::addProcessor(DataProcessor *proc)
{
    QMutexLocker locker(&s_mutex);
    if (!s_processors.contains(proc))
        s_processors.append(proc);
}

void DataDispatch::removeProcessor(DataProcessor *proc)
{
    QMutexLocker locker(&s_mutex);
    s_processors.removeOne(proc);
}

DataDispatch::SinkPtr DataDispatch::addDirectSink(std::function<void(const QByteArray&, int)> sink)
{
    auto ptr = std::make_shared<std::function<void(const QByteArray&, int)>>(std::move(sink));
    QMutexLocker locker(&s_mutex);
    s_directSinks.append(ptr);
    return ptr;
}

void DataDispatch::removeDirectSink(const SinkPtr &sinkPtr)
{
    QMutexLocker locker(&s_mutex);
    s_directSinks.removeOne(sinkPtr);
}

void DataDispatch::dataPacketCallback(const char *data, int length, int deviceId)
{

    QByteArray qba(data, length);
    delete[] data;

    QList<DataProcessor*> procs;
    QList<SinkPtr> sinks;
    {
        QMutexLocker locker(&s_mutex);
        procs = s_processors;
        sinks = s_directSinks;
    }

    for (auto *proc : procs) {
        QMetaObject::invokeMethod(proc, [=]() {
            proc->enqueueData(qba, deviceId);
        }, Qt::QueuedConnection);
    }
    for (auto &sinkPtr : sinks) {
        (*sinkPtr)(qba, deviceId);
    }
}
