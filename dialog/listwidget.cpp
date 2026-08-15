#include "listwidget.h"

listWidget::listWidget(QWidget *parent)
    : QListWidget(parent)
    , m_nMove(1)
    , m_bMouseStatusFlag(true)
{
    //    setHeaderHidden(true);
    //    setItemsExpandable(true);
    //    setRootIsDecorated(false);
    setLayoutDirection(Qt::LeftToRight);
    setVerticalScrollMode(QListWidget::ScrollPerPixel);
}

void listWidget::mouseMoveEvent(QMouseEvent *event)
{
    int move = height() - event->pos().y();
    int temp = move - m_nMove;
    m_bMouseStatusFlag = false;
    if (m_nMove > move) {
        verticalScrollBar()->setSliderPosition(verticalScrollBar()->value() + temp);

    } else if (m_nMove < move) {
        verticalScrollBar()->setSliderPosition(verticalScrollBar()->value() + temp);
    }
    m_nMove = move;
}

void listWidget::mousePressEvent(QMouseEvent *event)
{
    m_bMouseStatusFlag = true;
    m_nMove = height() - event->pos().y();
    QListWidget::mousePressEvent(event);
}

void listWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_bMouseStatusFlag) {
        event->accept();
        QListWidget::mouseReleaseEvent(event);
    } else {
        event->ignore();
    }
}
