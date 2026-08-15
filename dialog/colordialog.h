#ifndef COLORDIALOG_H
#define COLORDIALOG_H

#include "qbuttongroup.h"
#include <QDialog>
#include "listwidget.h"
#include <QDomDocument>
namespace Ui {
class ColorDialog;
}

class ColorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ColorDialog(const QString &colorFilesPath, QWidget *parent = nullptr);
    ~ColorDialog();

    const QString &getColorFile(void) const;
    static bool NdtLoadColorMap(QVector<QColor> &colorMap, const QString &file);

private:
    virtual void changeEvent(QEvent *event) Q_DECL_OVERRIDE;
private slots:
    void slotButtonClicked(QAbstractButton *btn);

    void listWidget_currentRowChanged(int currentRow);

private:
    Ui::ColorDialog *ui;
    QButtonGroup *btnGroup;
    QString choosedFile;
    QString filsPath;
    QDomDocument document;
    listWidget *colorListWidget;

    void findAllFiles(const QString &path);
    QStringList allColorfiles;
    QStringList validColorfiles;
};

#endif // COLORDIALOG_H
