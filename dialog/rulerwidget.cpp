#include "rulerwidget.h"
#include <QPainter>
#include "qdebug.h"
#include "ui_rulerwidget.h"

RulerWidget::RulerWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RulerWidget)
{
    ui->setupUi(this);
}

RulerWidget::~RulerWidget()
{
    delete ui;
}

void RulerWidget::setPosStart(double start)
{
    posStart = start;
    update();
}

double RulerWidget::getPosStart()
{

    return posStart;
}

void RulerWidget::setPosEnd(double end)
{
    posEnd = end;
    update();
}

double RulerWidget::getPosEnd()
{
    return posEnd;
}

void RulerWidget::setRulerUnit(QString unit)
{
    ruler_unit = unit;
}

void RulerWidget::setDirections(Directions index)
{
    m_vertical = index;
    update();
}

void RulerWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    double range = posEnd - posStart;
    if (range == 0)
        return;

    if (m_vertical == Directions::Horizontal) {

        int rulerWidth = width();
        // int rulerHeight = height();

        const int tickLengthMajor = 12;
        const int tickLengthMinor = 6;
        const int labelPadding = 3;
        const int minLabelGap = 50;

        double step = calculateOptimalStep(range, rulerWidth, 20);
        double startPos = floor(posStart / step) * step;
        if (startPos < posStart)
            startPos += step;

        QFontMetrics fm(painter.font());
        int lastLabelX = -minLabelGap;
        bool isFirstMajorTick = true;

        double minorStep = step / 5.0;
        double firstPos = startPos;
        while (firstPos > posStart + 1e-9)
            firstPos -= minorStep;

        for (auto pos = firstPos; pos <= posEnd + 1e-9; pos += minorStep) {
            if (pos < posStart - 1e-9) continue;
            double x = (pos - posStart) * (rulerWidth / range);

            double rem = fmod(pos, step);
            if (fabs(rem) < 1e-6 || step - fabs(rem) < 1e-6) {
                painter.drawLine(x, 0, x, tickLengthMajor);

                QString label = formatLabel(pos);
                int textWidth = fm.horizontalAdvance(label);
                int labelX = x - textWidth / 2;
                int labelY = tickLengthMajor + labelPadding + fm.ascent();

                labelX = std::max(0, labelX);
                labelX = std::min(labelX, rulerWidth - textWidth);

                if (labelX - textWidth / 2 > lastLabelX) {
                    painter.drawText(labelX, labelY, label);
                    lastLabelX = labelX + textWidth;

                    if (isFirstMajorTick) {
                        int unitY = labelY;
                        int unitX = x + fm.horizontalAdvance(ruler_unit) / 2;
                        unitX = std::max(0, unitX);
                        unitX = std::min(unitX, rulerWidth - fm.horizontalAdvance(ruler_unit));
                        painter.drawText(unitX, unitY, ruler_unit);
                        isFirstMajorTick = false;
                    }
                }
            } else {
                painter.drawLine(x, 0, x, tickLengthMinor);
            }
        }
    } else if (m_vertical == Directions::Vertical_left) {
        int rulerHeight = height();
        int rulerWidth = width();

        const int tickLengthMajor = 10;
        const int tickLengthMinor = 5;
        const int labelPadding = 3;
        const int minLabelGap = 20;

        double step = calculateOptimalStep(range, rulerHeight, 10);
        double startPos = floor(posStart / step) * step;
        if (startPos < posStart)
            startPos += step;

        QFontMetrics fm(painter.font());
        int lastLabelY = -minLabelGap;
        bool isFirstMajorTick = true;

        double minorStep = step / 5.0;
        double firstPos = startPos;
        while (firstPos > posStart + 1e-9)
            firstPos -= minorStep;

        for (auto pos = firstPos; pos <= posEnd + 1e-9; pos += minorStep) {
            if (pos < posStart - 1e-9) continue;
            double y = (pos - posStart) * (rulerHeight / range);

            double rem = fmod(pos, step);
            if (fabs(rem) < 1e-6 || step - fabs(rem) < 1e-6) {
                painter.drawLine(rulerWidth, y, rulerWidth - tickLengthMajor, y);

                QString label = formatLabel(pos);
                int textWidth = fm.horizontalAdvance(label);
                int labelX = rulerWidth - tickLengthMajor - labelPadding - textWidth;
                int labelY = y + fm.ascent() / 2;

                labelY = std::max(fm.ascent(), labelY);
                labelY = std::min(labelY, rulerHeight - fm.descent());

                if (labelY - fm.ascent() > lastLabelY) {
                    painter.drawText(labelX, labelY, label);
                    lastLabelY = labelY + fm.descent();

                    if (isFirstMajorTick && !ruler_unit.isEmpty()) {
                        int unitX = labelX - fm.horizontalAdvance(ruler_unit) + 10;
                        int unitY = labelY + 10;
                        painter.drawText(unitX, unitY, ruler_unit);
                        isFirstMajorTick = false;
                    }
                }
            } else {
                painter.drawLine(rulerWidth, y, rulerWidth - tickLengthMinor, y);
            }
        }
    } else if (m_vertical == Directions::Vertical_right) {
        int rulerHeight = height();
        int rulerWidth = width();

        const int tickLengthMajor = 10;
        const int tickLengthMinor = 5;
        const int labelPadding = 3;
        const int minLabelGap = 10;

        double step = calculateOptimalStep(range, rulerHeight, 10);
        double startPos = floor(posStart / step) * step;
        if (startPos < posStart)
            startPos += step;

        QFontMetrics fm(painter.font());
        int lastLabelY = rulerHeight + minLabelGap; // 改为从底部开始
        bool isFirstMajorTick = true;

        double minorStep = step / 5.0;
        double firstPos = startPos;
        while (firstPos > posStart + 1e-9)
            firstPos -= minorStep;

        for (auto pos = firstPos; pos <= posEnd + 1e-9; pos += minorStep) {
            if (pos < posStart - 1e-9) continue;

            // 计算y坐标（从底部开始）
            double y = rulerHeight - (pos - posStart) * (rulerHeight / range);

            double rem = fmod(pos, step);
            if (fabs(rem) < 1e-6 || step - fabs(rem) < 1e-6) {
                // 主刻度线（在左侧）
                painter.drawLine(0, y, tickLengthMajor, y);

                QString label = formatLabel(pos);
                int textWidth = fm.horizontalAdvance(label);
                int labelX = tickLengthMajor + labelPadding;
                int labelY = y + fm.ascent() / 2;

                // 确保标签不会超出边界
                labelY = std::max(fm.ascent(), labelY);
                labelY = std::min(labelY, rulerHeight - fm.descent());

                // 检查标签间距
                if (lastLabelY - (labelY + fm.descent()) > minLabelGap || isFirstMajorTick) {
                    painter.drawText(labelX, labelY, label);
                    lastLabelY = labelY - fm.descent();

                    if (isFirstMajorTick && !ruler_unit.isEmpty()) {
                        // 在第一个标签右边显示单位
                        int unitX = labelX + textWidth; // 在标签右边留出5像素间距
                        int unitY = labelY;
                        painter.drawText(unitX, unitY, ruler_unit);
                        isFirstMajorTick = false;
                    }
                }
            } else {
                // 次刻度线（在左侧）
                painter.drawLine(0, y, tickLengthMinor, y);
            }
        }
    }
}

double RulerWidget::calculateOptimalStep(double range, int rulerLength, int tick)
{
    const double baseSteps[] = { 1, 2, 5, 10, 20, 50, 100, 1000 };

    if (range <= 0 || rulerLength <= 0) {
        return 1.0;
    }

    int desiredTicks = qBound(tick, rulerLength / 100, 1000);

    double roughStep = range / desiredTicks;
    if (roughStep <= 0) {
        return 1.0;
    }

    double magnitude = pow(10, floor(log10(roughStep)));
    if (!std::isfinite(magnitude) || magnitude <= 0)
        magnitude = 1.0;

    double normalized = roughStep / magnitude;
    for (double step : baseSteps) {
        if (normalized <= step + 1e-6) {
            return step * magnitude;
        }
    }
    return 10 * magnitude;
}

QString RulerWidget::formatLabel(double value)
{
    if (abs(value) >= 1000) {
        return QString::number(value / 1000, 'f', 1) + "k";
    }
    return QString::number(value, 'f', (value == floor(value)) ? 0 : 1);
}
