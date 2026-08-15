#ifndef AXIS_UTILS_H
#define AXIS_UTILS_H

#include <deque>
#include <QString>

//存储坐标轴上的刻度pos以及上面对应的label
//函数需要在cpp实现
namespace AxisInfos {
struct AxisInfo
{
    QString label;
    double pos;
};

std::deque<AxisInfo *> generateAxisInfo(double startPos, double range, int tick_count, double area,
                                        QString unit = "mm", bool hasUnit = 1);

}
#endif // AXIS_UTILS_H
