#ifndef VIEWWORKER_H
#define VIEWWORKER_H

#include <QFile>
#include <QImage>
#include <QObject>
#include <QPointF>
#include <QRect>
#include <QTextStream>
#include <QVector>
#include <QRgb>

class ViewWorker : public QObject
{
    Q_OBJECT

public:
    explicit ViewWorker(QObject *parent = nullptr);
    ~ViewWorker() override;

public slots:
    void updateConfig(int pointQuantity, double maxAmp, int groupdata_size,
                      double range_start, double range_end, double rePointQuantity);
    void updateSoundConfig(double soundStart, double soundEnd);
    void updateCScanDataType(int dataType, bool isAmp);
    void setColorPalette(const QVector<QRgb> &colors);

    void processAScan(const QByteArray &data, int beamCount, int id_beam, bool replay);

    void processEScan(const QByteArray &data, int w, int h, bool replay);

    void processCScanFrame(int frameNo, int y, int h,
                           const QByteArray &datapack, int dataType, bool isAmp);

    void resizeCScanBuffers(int width, int height, int beamCount);
    void renderCScan();
    void recolorCScan();
    void resetCScanFrameCache();
    void setCScanViewIsAmp(bool isAmp);


signals:
    void aScanReady(QList<QPointF> points);
    void eScanReady(QImage image);
    void cScanImageReady(QImage image);

private:
    int m_pointQuantity = 1024;
    double m_maxAmp = 4096.0;
    int m_groupdata_size = 0;
    double m_range_start = 0.0;
    double m_range_end = 50.0;
    double m_rePointQuantity = 1.0 / 1024;

    double m_soundStart = 0.0;
    double m_soundEnd = 100.0;
    int m_dataType = 0;
    bool m_isAmp = true;
    QVector<QRgb> m_colorPalette;

    // C扫视图类型（true=C1-幅值, false=C2-声程）
    bool m_viewIsAmp = true;

    // C扫缓冲区
    QVector<int> m_cScanAmpBuffer;
    QVector<int> m_cScanSoundBuffer;
    int m_bufferWidth = 0;
    int m_bufferHeight = 0;
    QImage m_displayImage;
    QRect m_dirtyRect;
    bool m_hasDirtyRect = false;
    bool m_batchUpdateInProgress = false;
    bool m_renderPending = false;
    QVector<uint64_t> m_frameTimestamp;
    QVector<int> m_lastFrameUpdated;

    // 帧时间戳日志

};

#endif
