#include "viewworker.h"

#include <QtMath>
#include <QCoreApplication>
#include <QDateTime>
#include <cstdint>
#include <cstring>

struct CScan_MeasureValue
{
    uint64_t gateBAmplitude : 16;
    uint64_t gateBPositon : 24;
    uint64_t gateAAmplitude : 16;
    uint64_t gateAPositon_low : 8;

    uint64_t gateAPositon_high : 16;
    uint64_t gateIAmplitude : 16;
    uint64_t gateIPositon : 16;
    uint64_t gateIMaxPositon : 16;

    uint64_t gateCAmplitude_high : 8;
    uint64_t gateCPositon : 24;
    uint64_t gateDAmplitude : 16;
    uint64_t gateDPositon_low : 8;

    uint64_t gateDPositon_high : 16;
    uint64_t gateEAmplitude : 16;
    uint64_t gateEPositon : 24;
    uint64_t reverse : 8;
};

struct DataPacketTail
{
    int16_t frameErr : 1;
    int16_t scannerIO : 3;
    int16_t multiFrame : 12;
    uint16_t frameNo : 12;
    uint16_t encoder_trig_err : 2;
    uint16_t encoder_sync_err : 2;
    int32_t encoderA_pos : 32;
    int32_t encoderB_pos : 32;
    int32_t encoderC_pos : 32;
    int32_t encoderD_pos : 32;
    int32_t encoderE_pos : 32;
    uint64_t timestamp : 64;
};

ViewWorker::ViewWorker(QObject *parent)
    : QObject(parent)
{
}

ViewWorker::~ViewWorker()
{
}

void ViewWorker::updateConfig(int pointQuantity, double maxAmp, int groupdata_size,
                              double range_start, double range_end, double rePointQuantity)
{
    m_pointQuantity = pointQuantity;
    m_maxAmp = maxAmp;
    m_groupdata_size = groupdata_size;
    m_range_start = range_start;
    m_range_end = range_end;
    m_rePointQuantity = rePointQuantity;
}

void ViewWorker::updateSoundConfig(double soundStart, double soundEnd)
{
    m_soundStart = soundStart;
    m_soundEnd = soundEnd;
}

void ViewWorker::updateCScanDataType(int dataType, bool isAmp)
{
    m_dataType = dataType;
    m_isAmp = isAmp;
}

void ViewWorker::setColorPalette(const QVector<QRgb> &colors)
{
    m_colorPalette = colors;
}

void ViewWorker::processAScan(const QByteArray &data, int beamCount, int id_beam, bool replay)
{
    if (data.isEmpty() || beamCount <= 0)
        return;

    const int16_t *dataPtr = reinterpret_cast<const int16_t *>(data.constData());
    auto beam_point = m_pointQuantity;
    auto cnt_points_1 = beam_point / 8 * 8;
    auto max_size = beamCount * cnt_points_1;

    if (static_cast<long long>(data.size()) < (static_cast<long long>(m_groupdata_size) + static_cast<long long>(beamCount) * (beam_point + 16)) * sizeof(int16_t))
        return;

    QVector<int16_t> imageData(max_size);
    int groupOffset = replay ? 0 : m_groupdata_size;
    for (int i = 0; i < beamCount; i++)
        for (int j = 0; j < cnt_points_1; j++)
            imageData[j + i * cnt_points_1] = dataPtr[groupOffset + j + i * (beam_point + 16)];

    QList<QPointF> points;
    const int cnt = cnt_points_1;
    int start = (id_beam - 1) * cnt_points_1;
    for (int i = 0; i < cnt && start < imageData.size(); i++, start++)
        points.append(QPointF(static_cast<qreal>(i),
                              static_cast<qreal>(imageData[start] * 1.0 / m_maxAmp * 100)));

    emit aScanReady(points);
}

void ViewWorker::processEScan(const QByteArray &src, int w, int h, bool replay)
{
    if (w <= 0 || h <= 0 || m_colorPalette.isEmpty())
        return;

    int groupOffset = replay ? 0 : m_groupdata_size;
    if (static_cast<long long>(src.size()) < (static_cast<long long>(groupOffset) + static_cast<long long>(h) * (w + 16)) * sizeof(int16_t))
        return;

    const int16_t *dataPtr = reinterpret_cast<const int16_t *>(src.constData());

    // 与原始 showEScan 一致的像素布局：
    // image(w=h_beams, h=w_points), scanLine(j)[i] = data[beam=i][point=j]
    QImage image(h, w, QImage::Format_ARGB32);
    if (image.isNull())
        return;

    int colorSize = m_colorPalette.size();
    for (int i = 0; i < h; i++) {
        int pos1 = i * (w + 16);
        for (int j = 0; j < w; j++) {
            int pos2 = static_cast<int>(qAbs(dataPtr[groupOffset + pos1 + j] * 1.0 / m_maxAmp * 255));
            pos2 = qBound(0, pos2, 255);
            uint32_t *targetLine = reinterpret_cast<uint32_t *>(image.scanLine(j));
            targetLine[i] = m_colorPalette[qMin(pos2, colorSize - 1)];
        }
    }

    emit eScanReady(image);
}

void ViewWorker::processCScanFrame(int frameNo, int y, int h,
                                   const QByteArray &datapack, int dataType, bool isAmp)
{
    if (frameNo < 0 || h <= 0 || m_pointQuantity <= 0)
        return;
    if (static_cast<long long>(datapack.size()) < static_cast<long long>(h) * (m_pointQuantity + 16) * sizeof(int16_t) + static_cast<long long>(sizeof(CScan_MeasureValue)))
        return;

    // 时间戳去重
    uint64_t currentTimestamp = 0;
    if (datapack.size() >= static_cast<int>(sizeof(DataPacketTail))) {
        DataPacketTail tail;
        memcpy(&tail, datapack.constData() + datapack.size() - sizeof(tail), sizeof(tail));
        currentTimestamp = tail.timestamp;
    }
    if (frameNo >= 0 && frameNo < m_frameTimestamp.size()) {
        if (currentTimestamp > 0 && m_frameTimestamp[frameNo] >= currentTimestamp)
            return;
    }
    if (frameNo >= 0 && frameNo < m_lastFrameUpdated.size()) {
        if (m_lastFrameUpdated[frameNo] >= frameNo)
            return;
    }

    m_frameTimestamp[frameNo] = currentTimestamp;
    m_lastFrameUpdated[frameNo] = frameNo;

    if (frameNo < 0 || frameNo >= m_bufferWidth)
        return;
    if (y < 0 || y + h > m_bufferHeight)
        return;

    QVector<int> &buffer = isAmp ? m_cScanAmpBuffer : m_cScanSoundBuffer;
    if (buffer.isEmpty() || buffer.size() < m_bufferWidth * m_bufferHeight) {
        buffer.resize(m_bufferWidth * m_bufferHeight);
        buffer.fill(0);
    }

    CScan_MeasureValue measureInform;
    double soundRange = m_soundEnd - m_soundStart;
    bool hasChanges = false;

    for (int i = 0; i < h; i++) {
        int targetY = i + y;
        if (targetY < 0 || targetY >= m_bufferHeight) continue;

        auto current = (m_pointQuantity * (i + 1) + 16 * i) * sizeof(int16_t);
        memcpy(&measureInform, datapack.data() + current, sizeof(measureInform));

        int colorIndex = 0;

        if (isAmp) {
            double ampValue = 0;
            switch (dataType) {
            case 0: ampValue = measureInform.gateAAmplitude; break;
            case 1: ampValue = measureInform.gateBAmplitude; break;
            case 2: ampValue = (measureInform.gateCAmplitude_high << 8); break;
            case 3: ampValue = measureInform.gateIAmplitude; break;
            }
            double percentage = ampValue * 100.0 / m_maxAmp;
            colorIndex = qBound(0, static_cast<int>(percentage * 2.55), 255);
        } else {
            double southValue = 0;
            switch (dataType) {
            case 4:
                southValue = (measureInform.gateAPositon_high * 256 + measureInform.gateAPositon_low)
                    * m_rePointQuantity * (m_range_end - m_range_start) + m_range_start;
                break;
            case 5:
                southValue = measureInform.gateBPositon * m_rePointQuantity * (m_range_end - m_range_start) + m_range_start;
                break;
            case 6:
                southValue = measureInform.gateCPositon * m_rePointQuantity * (m_range_end - m_range_start) + m_range_start;
                break;
            case 7:
                southValue = measureInform.gateIPositon * m_rePointQuantity * (m_range_end - m_range_start) + m_range_start;
                break;
            }
            if (soundRange > 0) {
                double ratio = (southValue - m_soundStart) / soundRange;
                colorIndex = (ratio >= 1.0) ? 255 : (ratio <= 0.0) ? 0 : qBound(1, static_cast<int>(ratio * 255), 254);
            }
        }

        int index = targetY * m_bufferWidth + frameNo;
        if (index >= 0 && index < buffer.size()) {
            if (buffer[index] != colorIndex) {
                buffer[index] = colorIndex;
                hasChanges = true;
            }
        }
    }

    if (hasChanges) {
        QRect newRect(frameNo, y, 1, h);
        m_dirtyRect = m_hasDirtyRect ? m_dirtyRect.united(newRect) : newRect;
        m_hasDirtyRect = true;
    }

    if (m_hasDirtyRect && !m_renderPending) {
        m_renderPending = true;
        QMetaObject::invokeMethod(this, "renderCScan", Qt::QueuedConnection);
    }
}

void ViewWorker::resizeCScanBuffers(int width, int height, int beamCount)
{
    m_bufferWidth = width;
    m_bufferHeight = height;

    m_cScanAmpBuffer.clear();
    m_cScanSoundBuffer.clear();

    int bufferSize = width * height;
    if (bufferSize > 0) {
        try {
            m_cScanAmpBuffer.resize(bufferSize);
            m_cScanAmpBuffer.fill(0);
            m_cScanSoundBuffer.resize(bufferSize);
            m_cScanSoundBuffer.fill(0);
        } catch (const std::bad_alloc &) {
            m_cScanAmpBuffer.clear();
            m_cScanSoundBuffer.clear();
            return;
        }
    }

    m_displayImage = QImage();
    if (width > 0 && height > 0) {
        m_displayImage = QImage(width, height, QImage::Format_ARGB32);
        m_displayImage.fill(Qt::white);
    }

    m_frameTimestamp.resize(width);
    m_frameTimestamp.fill(0);
    m_lastFrameUpdated.resize(width);
    m_lastFrameUpdated.fill(-1);

    m_hasDirtyRect = false;
    m_renderPending = false;

    emit cScanImageReady(m_displayImage);
}

void ViewWorker::renderCScan()
{
    if (m_batchUpdateInProgress) return;
    m_batchUpdateInProgress = true;

    if (!m_hasDirtyRect) {
        m_batchUpdateInProgress = false;
        return;
    }

    QRect dirtyRect = m_dirtyRect;
    m_hasDirtyRect = false;
    m_renderPending = false;

    if (m_displayImage.isNull() || m_displayImage.width() != m_bufferWidth
        || m_displayImage.height() != m_bufferHeight) {
        m_displayImage = QImage(m_bufferWidth, m_bufferHeight, QImage::Format_ARGB32);
        m_displayImage.fill(Qt::white);
    }

    int xStart = qBound(0, dirtyRect.left(), m_bufferWidth - 1);
    int xEnd = qBound(xStart, dirtyRect.right(), m_bufferWidth - 1);
    int yStart = qBound(0, dirtyRect.top(), m_bufferHeight - 1);
    int yEnd = qBound(yStart, dirtyRect.bottom(), m_bufferHeight - 1);

    if (m_colorPalette.isEmpty() || m_bufferWidth <= 0 || m_bufferHeight <= 0) {
        m_batchUpdateInProgress = false;
        return;
    }

    QVector<int> &buffer = m_viewIsAmp ? m_cScanAmpBuffer : m_cScanSoundBuffer;
    int colorSize = m_colorPalette.size();
    for (int y = yStart; y <= yEnd; y++) {
        if (y < 0 || y >= m_displayImage.height()) continue;
        uint32_t *scanLine = reinterpret_cast<uint32_t *>(m_displayImage.scanLine(y));
        int bufferOffset = y * m_bufferWidth;
        for (int x = xStart; x <= xEnd; x++) {
            int idx = bufferOffset + x;
            if (idx >= 0 && idx < buffer.size()) {
                int ci = buffer[idx];
                if (ci >= 0 && ci < colorSize)
                    scanLine[x] = m_colorPalette[ci];
            }
        }
    }

    m_batchUpdateInProgress = false;

    emit cScanImageReady(m_displayImage);

    if (m_hasDirtyRect) {
        QMetaObject::invokeMethod(this, "renderCScan", Qt::QueuedConnection);
    }
}

void ViewWorker::recolorCScan()
{
    m_dirtyRect = QRect(0, 0, m_bufferWidth, m_bufferHeight);
    m_hasDirtyRect = true;
    if (!m_renderPending) {
        m_renderPending = true;
        QMetaObject::invokeMethod(this, "renderCScan", Qt::QueuedConnection);
    }
}

void ViewWorker::resetCScanFrameCache()
{
    m_frameTimestamp.fill(0);
    m_lastFrameUpdated.fill(-1);
}

void ViewWorker::setCScanViewIsAmp(bool isAmp)
{
    m_viewIsAmp = isAmp;
    m_dirtyRect = QRect(0, 0, m_bufferWidth, m_bufferHeight);
    m_hasDirtyRect = true;
    if (!m_renderPending) {
        m_renderPending = true;
        QMetaObject::invokeMethod(this, "renderCScan", Qt::QueuedConnection);
    }
}


