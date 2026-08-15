#ifndef ACG_TCG_WIDGET_H
#define ACG_TCG_WIDGET_H

#include "qpushbutton.h"
#include <QSpinBox>
#include <QWidget>
#include <QVector>
#include <QPair>
#include <client.h>
#include <app.h>

namespace Ui {
class ACG_TCG_widget;
}

class ACG_TCG_widget : public QWidget
{
    Q_OBJECT

public:
    explicit ACG_TCG_widget(QWidget *parent = nullptr);
    ~ACG_TCG_widget();

private:
    void initWidget();
    void refreshTabwidget();
    int findNextAvailableIndex(const std::vector<int> &numbers);
    void loadTcgPoints();
    int beamFromRow(int row) const { return row >= 0 && row < m_tcgPoints.size() ? m_tcgPoints[row].second : -1; }
    int indexFromRow(int row) const { return row >= 0 && row < m_tcgPoints.size() ? m_tcgPoints[row].first : -1; }
    std::vector<std::pair<int, int>> m_tcgPoints;  // (index, beamIndex)
private slots:
    void on_TCG_ON_Btn_clicked();
    void on_Add_point_Btn_clicked();
    void handleBeamSpinBoxValueChanged(int beam);

    void onCellChanged(int row, int column);
    void onDeleteRow();

protected:
    virtual void changeEvent(QEvent *event) Q_DECL_OVERRIDE;
    virtual void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;

private:
    Ui::ACG_TCG_widget *ui;
    QPushButton *deleteButton;
    Client &config;
    bool tcg_enable = false;
    int m_currentBeam = 1;
};

#endif // ACG_TCG_WIDGET_H
