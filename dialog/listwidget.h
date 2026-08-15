#ifndef TREEWIDGET_H
#define TREEWIDGET_H

#include <QListWidget>
#include <QScrollBar>
#include <QMouseEvent>

class listWidget : public QListWidget
{
    Q_OBJECT
public:
    listWidget(QWidget *parent = nullptr);

protected:
    void mouseMoveEvent(QMouseEvent *);

    void mousePressEvent(QMouseEvent *);

    void mouseReleaseEvent(QMouseEvent *);

private:
    QPoint m_PressPoint;
    int m_nMove;
    bool m_bMouseStatusFlag;
};

#endif // TREEWIDGET_H
