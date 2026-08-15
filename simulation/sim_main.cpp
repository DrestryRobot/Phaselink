#include "SimDataPlayer.h"

#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QTimer>

#include <cstdio>

// ============================================================
// 独立模拟器：在其他设备上以真实帧率回放录制 CSV，
// 产生与真实设备扫描时相同的 scandata 全局量。
// 用法：
//   SimDataPlayer.exe [csv1] [csv2] ...
// 不传参数时默认使用本机 Downloads 下的两个录制文件。
// ============================================================

// scandata 全局量定义（独立构建时由本文件提供）
double amp[64] = {0};
double tof[64] = {0};
bool beamValid[64] = {false};
double si = 0.0;
int beam = 49;
double robot_x = 0, robot_y = 0, robot_z = 0;
double robot_a = 0, robot_b = 0, robot_c = 0;
quint32 robot_ipoc = 0;
double longmen[2] = {0, 0};
bool m_start = false;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    fprintf(stderr, "[Sim] SimDataPlayer 启动\n");
    fflush(stderr);

    QStringList files;
    for (int i = 1; i < argc; i++)
        files << QString::fromLocal8Bit(argv[i]);
    if (files.isEmpty()) {
        files << "C:/Users/23714/Downloads/scan_20260717_141034.csv"
              << "C:/Users/23714/Downloads/scan_20260717_133933.csv";
    }

    SimDataPlayer player;
    if (!player.loadCsv(files)) {
        qWarning() << "加载录制文件失败";
        return 1;
    }
    fprintf(stderr, "[Sim] 加载完成，总行数: %lld\n", (long long)player.totalRows());
    fflush(stderr);

    // 每秒打印一次模拟状态（与真实设备日志风格一致）
    QTimer statTimer;
    statTimer.setInterval(1000);
    QObject::connect(&statTimer, &QTimer::timeout, [&]() {
        qDebug() << QString("[Sim] 回放: %1/%2  帧率: %3Hz  IPOC: %4  位姿: %5 %6 %7  SI: %8  AMP0: %9  m_start: %10")
                        .arg(player.replayedRows())
                        .arg(player.totalRows())
                        .arg(player.replayedRows() ? "250" : "0")
                        .arg(robot_ipoc)
                        .arg(robot_x, 0, 'f', 1)
                        .arg(robot_y, 0, 'f', 1)
                        .arg(robot_z, 0, 'f', 1)
                        .arg(si, 0, 'f', 3)
                        .arg(amp[0], 0, 'f', 4)
                        .arg(m_start ? "true" : "false");
        fprintf(stderr, "[Sim] 回放: %lld/%lld IPOC: %u 位姿: %.1f %.1f %.1f SI: %.3f AMP0: %.4f m_start: %d\n",
                (long long)player.replayedRows(), (long long)player.totalRows(),
                robot_ipoc, robot_x, robot_y, robot_z, si, amp[0], m_start ? 1 : 0);
        fflush(stderr);
    });
    statTimer.start();

    QObject::connect(&player, &SimDataPlayer::finished, &app, [&]() {
        qDebug() << "[Sim] 模拟结束";
        QTimer::singleShot(200, &app, &QCoreApplication::quit);
    });

    player.start();
    return app.exec();
}
