#ifndef ADDDEVICEDIALOG_H
#define ADDDEVICEDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QRegularExpression>
#include <QIntValidator>

class AddDeviceDialog : public QDialog
{
    Q_OBJECT

public:
    AddDeviceDialog(QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(tr("Add Device"));
        setModal(true);
        setFixedSize(300, 150);

        // 创建控件
        QLabel *ipLabel = new QLabel(tr("IP Address:"));
        QLabel *idLabel = new QLabel(tr("Device ID:"));
        ipEdit = new QLineEdit;
        idEdit = new QLineEdit;
        idEdit->setText("1"); // 默认ID为1

        QPushButton *okButton = new QPushButton(tr("OK"));
        QPushButton *cancelButton = new QPushButton(tr("Cancel"));

        // 布局
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        QHBoxLayout *ipLayout = new QHBoxLayout;
        QHBoxLayout *idLayout = new QHBoxLayout;
        QHBoxLayout *buttonLayout = new QHBoxLayout;

        ipLayout->addWidget(ipLabel);
        ipLayout->addWidget(ipEdit);
        idLayout->addWidget(idLabel);
        idLayout->addWidget(idEdit);
        buttonLayout->addWidget(okButton);
        buttonLayout->addWidget(cancelButton);

        mainLayout->addLayout(ipLayout);
        mainLayout->addLayout(idLayout);
        mainLayout->addLayout(buttonLayout);

        // 连接信号
        connect(okButton, &QPushButton::clicked, this, &AddDeviceDialog::onOkClicked);
        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

        // 设置验证器
        idEdit->setValidator(new QIntValidator(1, 9999, this));
    }

    QString getIpAddress() const { return ipEdit->text().trimmed(); }
    int getDeviceId() const { return idEdit->text().toInt(); }

private slots:
    void onOkClicked()
    {
        QString ip = getIpAddress();
        int id = getDeviceId();

        // 验证IP格式
        if (ip.isEmpty()) {
            QMessageBox::warning(this, tr("Warning"), tr("Please enter IP address!"));
            return;
        }

        QRegularExpression ipRegex("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4]["
                                   "0-9]|[01]?[0-9][0-9]?)$");
        if (!ipRegex.match(ip).hasMatch()) {
            QMessageBox::warning(this, tr("Warning"), tr("Invalid IP address format!"));
            return;
        }

        if (id < 1) {
            QMessageBox::warning(this, tr("Warning"), tr("Device ID must be greater than 0!"));
            return;
        }

        accept();
    }

private:
    QLineEdit *ipEdit;
    QLineEdit *idEdit;
};

#endif // ADDDEVICEDIALOG_H
