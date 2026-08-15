#include "axis_utils.h"

std::deque<AxisInfos::AxisInfo *> AxisInfos::generateAxisInfo(double startPos, double range,
                                                              int tick_count, double area,
                                                              QString unit, bool hasUnit)
{
    std::deque<AxisInfo *> axisInfos;

    double tickInterval = range / (tick_count - 1);
    double areaInterval = area / (tick_count - 1);

    for (int i = 0; i < tick_count; i++) {
        double pos = startPos + i * tickInterval;
        double areavalue = startPos + i * areaInterval;

        AxisInfo *info = new AxisInfo();
        if (hasUnit)
            info->label = QString::number(areavalue, 'f', 1) + unit;
        else
            info->label = QString::number(areavalue, 'f', 1);
        info->pos = pos;
        axisInfos.push_back(info);
    }
    return axisInfos;
}
