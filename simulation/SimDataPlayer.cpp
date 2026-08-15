#include "SimDataPlayer.h"

#include <QDebug>

#include <cstdio>
#include <cstring>

SimDataPlayer::SimDataPlayer(QObject *parent)
    : QObject(parent)
{
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &SimDataPlayer::replayNextRow);
}

SimDataPlayer::~SimDataPlayer()
{
    stop();
}

bool SimDataPlayer::loadCsv(const QString &path)
{
    // 统计行数（跳过表头）
    std::ifstream counter(path.toStdString(), std::ios::in);
    if (!counter.is_open()) {
        qWarning() << "[SimDataPlayer] 无法打开文件:" << path;
        return false;
    }
    qint64 rows = 0;
    std::string line;
    bool first = true;
    while (std::getline(counter, line)) {
        if (first) { first = false; continue; }
        if (!line.empty()) rows++;
    }
    counter.close();

    CsvFile f;
    f.path = path;
    f.rows = rows;
    m_files.append(f);
    m_totalRows += rows;
    qDebug() << "[SimDataPlayer] 加载录制:" << path << " 数据行:" << rows;
    return true;
}

bool SimDataPlayer::loadCsv(const QStringList &paths)
{
    bool ok = true;
    for (const QString &p : paths)
        ok = loadCsv(p) && ok;
    return ok;
}

bool SimDataPlayer::isFinished() const
{
    return m_started && m_currentFile >= m_files.size();
}

void SimDataPlayer::start()
{
    if (m_started)
        return;
    if (m_files.isEmpty()) {
        qWarning() << "[SimDataPlayer] 没有已加载的录制文件";
        return;
    }
    m_started = true;
    m_replayedRows = 0;
    m_currentFile = -1;
    m_start = true;
    qDebug() << "[SimDataPlayer] 开始模拟，每行" << m_rowIntervalMs << "ms"
             << (m_rowIntervalMs == 4 ? "(250Hz)" : "(自定义帧率)");

    if (openNextFile())
        m_timer.start(m_rowIntervalMs);
    else
        finish();
}

void SimDataPlayer::stop()
{
    if (!m_started)
        return;
    m_started = false;
    m_start = false;
    m_timer.stop();
    qDebug() << "[SimDataPlayer] 停止模拟，已回放:" << m_replayedRows << "/" << m_totalRows;
}

bool SimDataPlayer::openNextFile()
{
    m_currentFile++;
    if (m_currentFile >= m_files.size())
        return false;

    CsvFile &f = m_files[m_currentFile];
    f.stream = std::make_shared<std::ifstream>(f.path.toStdString(), std::ios::in);
    if (!f.stream->is_open()) {
        qWarning() << "[SimDataPlayer] 无法打开:" << f.path;
        return openNextFile();
    }
    // 跳过表头
    std::string header;
    std::getline(*f.stream, header);
    f.headerSkipped = true;
    // 统计列数（按表头）
    int cols = 1;
    for (char ch : header)
        if (ch == ',') cols++;
    f.columnCount = cols;
    qDebug() << "[SimDataPlayer] 开始回放文件:" << f.path
             << " 列数:" << f.columnCount;
    return true;
}

bool SimDataPlayer::readNextRow(QVector<double> &cols)
{
    while (true) {
        if (m_currentFile < 0 || m_currentFile >= m_files.size())
            return false;
        CsvFile &f = m_files[m_currentFile];
        if (!f.stream || f.stream->eof()) {
            if (!openNextFile())
                return false;
            continue;
        }
        std::string line;
        if (!std::getline(*f.stream, line)) {
            if (!openNextFile())
                return false;
            continue;
        }
        if (line.empty())
            continue;   // 跳过空行

        // 去掉行尾 \r
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
            line.pop_back();

        cols.clear();
        size_t start = 0;
        while (true) {
            size_t comma = line.find(',', start);
            std::string field =
                (comma == std::string::npos) ? line.substr(start)
                                             : line.substr(start, comma - start);
            if (field.empty())
                cols.append(0.0);
            else
                cols.append(std::strtod(field.c_str(), nullptr));
            if (comma == std::string::npos)
                break;
            start = comma + 1;
        }
        return true;
    }
}

void SimDataPlayer::replayNextRow()
{
    QVector<double> cols;
    if (!readNextRow(cols)) {
        finish();
        return;
    }
    m_replayedRows++;
    applyRow(cols);
}

void SimDataPlayer::applyRow(const QVector<double> &c)
{
    if (c.size() < 8)
        return;

    // 机器人位姿
    robot_x = c[0];
    robot_y = c[1];
    robot_z = c[2];
    robot_a = c[3];
    robot_b = c[4];
    robot_c = c[5];
    // 声程 SI
    si = c[6];

    // 每波束 AMP/TOF（表头为 AMP_1,TOF_1,AMP_2,TOF_2,...）
    int nBeam = (c.size() - 7) / 2;
    if (nBeam > 64)
        nBeam = 64;
    for (int i = 0; i < nBeam; i++) {
        amp[i] = c[7 + 2 * i];
        tof[i] = c[8 + 2 * i];
    }
    for (int i = nBeam; i < 64; i++) {
        amp[i] = 0.0;
        tof[i] = 0.0;
    }

    // BEAM / LX / LY：录制时只有首行有值
    int col = 7 + 2 * nBeam;
    if (c.size() > col && c[col] != 0.0)
        beam = static_cast<int>(c[col]);
    if (c.size() > col + 1 && c[col + 1] != 0.0)
        longmen[0] = c[col + 1];
    if (c.size() > col + 2 && c[col + 2] != 0.0)
        longmen[1] = c[col + 2];
    if (beam <= 0 || beam > 64)
        beam = 49;

    // 波束有效性：板外/无效波束置 false
    for (int i = 0; i < 64; i++)
        beamValid[i] = (i < beam) && (amp[i] != 0.0 || tof[i] != 0.0);

    // IPOC：4ms 一帧，模拟 1kHz 硬件计数器（每帧 +4）
    m_ipocCounter += 4;
    robot_ipoc = m_ipocCounter;
}

void SimDataPlayer::finish()
{
    m_started = false;
    m_start = false;
    m_timer.stop();
    qDebug() << "[SimDataPlayer] 全部录制回放完成，总行数:" << m_replayedRows;
    emit finished();
}
