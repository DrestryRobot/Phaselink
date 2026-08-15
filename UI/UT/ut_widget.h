#ifndef UT_WIDGET_H
#define UT_WIDGET_H

#include <QWidget>

namespace Ui {
class UT_widget;
}

class UT_widget : public QWidget
{
    Q_OBJECT

public:
    explicit UT_widget(QWidget *parent = nullptr);
    ~UT_widget();

private:
    virtual void changeEvent(QEvent *event) Q_DECL_OVERRIDE;
private slots:

    void on_tabWidget_currentChanged(int index);

private:
    Ui::UT_widget *ui;
};

#endif // UT_WIDGET_H
