#ifndef SIMDATAPLAYER_H
#define SIMDATAPLAYER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVector>

#include <fstream>
#include <memory>

// ============================================================
// 与 3DScan/scandata.h 完全一致的共享全局量声明。
// 在 SoundScan 内集成时由 scandata.cpp 提供定义；
// 独立模拟器（sim_main.cpp）自行定义。
// ============================================================
extern double amp[64];
extern double tof[64];
extern bool beamValid[64];
extern double si;
extern int beam;

extern double robot_x, robot_y, robot_z;
extern double robot_a, robot_b, robot_c;
extern quint32 robot_ipoc;

extern double longmen[2];

extern bool m_start;

// ============================================================
// 录制 CSV 模拟播放器
// ------------------------------------------------
// 以与真实设备相同的帧率回放录制数据：
//   - 机器人：每行一帧，默认 4ms（250Hz），robot_ipoc 每帧 +4（模拟 1kHz 计数器）
//   - 超声：CSV 每行已携带与机器人同步的 AMP/TOF，逐行回放即完全还原录制序列
//   （录制时超声源约为 180Hz，其变化节奏已内嵌在 250Hz 行数据中）
// ============================================================
class SimDataPlayer : public QObject
{
    Q_OBJECT

public:
    explicit SimDataPlayer(QObject *parent = nullptr);
    ~SimDataPlayer() override;

    // 加载一个或多个录制文件（多个文件按顺序连续回放）
    bool loadCsv(const QString &path);
    bool loadCsv(const QStringList &paths);

    // 每行回放间隔（毫秒），默认 4ms = 250Hz
    void setRowIntervalMs(int ms) { m_rowIntervalMs = qMax(1, ms); }
    void setIpocBase(quint32 base) { m_ipocCounter = base; }

    qint64 totalRows() const { return m_totalRows; }
    qint64 replayedRows() const { return m_replayedRows; }
    bool isRunning() const { return m_started; }
    bool isFinished() const;

public slots:
    // 开始模拟：置 m_start=true 并启动 250Hz 逐行回放
    void start();
    // 停止模拟：置 m_start=false 并停止回放
    void stop();

signals:
    void finished();

private slots:
    void replayNextRow();

private:
    struct CsvFile
    {
        QString path;
        qint64 rows = 0;
        std::shared_ptr<std::ifstream> stream;
        bool headerSkipped = false;
        int columnCount = 0;
    };

    bool openNextFile();
    bool readNextRow(QVector<double> &cols);
    void applyRow(const QVector<double> &cols);
    void finish();

    QVector<CsvFile> m_files;
    int m_currentFile = -1;
    QTimer m_timer;

    qint64 m_totalRows = 0;
    qint64 m_replayedRows = 0;
    quint32 m_ipocCounter = 0;
    int m_rowIntervalMs = 4;   // 4ms = 250Hz
    bool m_started = false;
};

#endif // SIMDATAPLAYER_H
