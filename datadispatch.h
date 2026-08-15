#ifndef DATADISPATCH_H
#define DATADISPATCH_H

#include <QByteArray>
#include <QMutex>
#include <QList>
#include <functional>
#include <memory>

class DataProcessor;

class DataDispatch
{
public:
    using SinkPtr = std::shared_ptr<std::function<void(const QByteArray&, int)>>;

    static void addProcessor(DataProcessor *proc);
    static void removeProcessor(DataProcessor *proc);
    static SinkPtr addDirectSink(std::function<void(const QByteArray&, int)> sink);
    static void removeDirectSink(const SinkPtr &sinkPtr);
    static void dataPacketCallback(const char *data, int length, int deviceId);

private:
    static QList<DataProcessor*> s_processors;
    static QList<SinkPtr> s_directSinks;
    static QMutex s_mutex;
};

#endif // DATADISPATCH_H
