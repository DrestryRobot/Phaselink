#ifndef SIDER_H
#define SIDER_H

#include <QWidget>
#include <QScrollBar>

namespace Ui {
class Sider;
}

class Sider : public QWidget
{
    Q_OBJECT

public:
    explicit Sider(QWidget *parent = nullptr);
    ~Sider();
    QScrollBar *silder();
private slots:
    void on_horizontalScrollBar_valueChanged(int value);

private:
    Ui::Sider *ui;
};

#endif // SIDER_H
