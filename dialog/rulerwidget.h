#ifndef RULERWIDGET_H
#define RULERWIDGET_H

#include <QWidget>

enum class Directions {
    Horizontal,    //单波束
    Vertical_left, //双波束
    Vertical_right // 4 波束
};

namespace Ui {
class RulerWidget;
}

class RulerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RulerWidget(QWidget *parent = nullptr);
    ~RulerWidget();
    void setPosStart(double start);
    double getPosStart();
    void setPosEnd(double end);
    double getPosEnd();
    void setRulerUnit(QString unit);
    void setDirections(Directions index);

private:
    double calculateOptimalStep(double range, int rulerHeight, int tick);
    QString formatLabel(double value);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Ui::RulerWidget *ui;
    double posStart;
    double posEnd;
    QString ruler_unit = "mm";
    Directions m_vertical = Directions::Horizontal;
};

#endif // RULERWIDGET_H
