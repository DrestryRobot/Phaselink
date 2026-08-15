#ifndef AMPLITUDPALETTE_H
#define AMPLITUDPALETTE_H

#include <QWidget>
#include "colordialog.h"
#include <app.h>
namespace Ui {
class AmplitudPalette;
}

class AmplitudPalette : public QWidget
{
    Q_OBJECT

public:
    explicit AmplitudPalette(QWidget *parent = nullptr);
    ~AmplitudPalette();

    void setColors(const QVector<QRgb> &colors);
    const QVector<QRgb> &getColors() const;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override; // 双击事件

private:
    Ui::AmplitudPalette *ui;
    QVector<QRgb> m_colors;
};

#endif // AMPLITUDPALETTE_H
