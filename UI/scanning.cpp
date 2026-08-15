#include "scanning.h"
#include "ui_scanning.h"
#include <QJsonObject>
#include <QCoreApplication>

Scanning::Scanning(QWidget *parent)
    : NdtBase(parent)
    , ui(new Ui::Scanning)
    , config(Client::getInstance())
    , paramManager(ParamManager::getInstance())
    , isLoadingParams(false)
{
    ui->setupUi(this);
    initWidget();
    connect(App::getInstance(), &App::signal_xPosition, this,
            [=](double posoiton) { ui->X_position->setValue(posoiton); });
    // 加载保存的参数
    loadParameters();

    // 连接保存信号的槽
    connectSaveSignals();
}

Scanning::~Scanning()
{
    delete ui;
}

void Scanning::initWidget()
{
    this->setWindowFlags(Qt::FramelessWindowHint);
    ui->CscanData_Amp->setView(new QListView);
    ui->CscanData_TOF->setView(new QListView);
    ui->comboBox->setView(new QListView);
    ui->EncoderComboBox->setView(new QListView);
    ui->xDirCombox->setView(new QListView);
    ui->yDirCombox->setView(new QListView);
    ui->Resolution_value->setMinimum(0.001);
    ui->Resolution_value_2->setMinimum(0.001);
    ui->End_value->setMaximum(999999);
    ui->End_value_2->setMaximum(999999);
    // getCurrentPara();
}

void Scanning::getCurrentPara()
{

    ui->Resolution_value->blockSignals(true);
    ui->time_resolute_value->blockSignals(true);
    ui->spinResolution->blockSignals(true);
    ui->spinResolution_index->blockSignals(true);
    ui->Resolution_X->blockSignals(true);
    ui->End_value->blockSignals(true);
    ui->End_value_2->blockSignals(true);
    ui->time_lengt_value->blockSignals(true);
    ui->SoundStart_value->blockSignals(true);
    ui->SoundEnd_value->blockSignals(true);
    ui->Resolution_value_3->blockSignals(true);
    ui->End_value_3->blockSignals(true);

    ui->CscanData_Amp->blockSignals(true);
    ui->CscanData_TOF->blockSignals(true);

    ui->CscanData_TOF->setCurrentIndex(static_cast<int>(App::getInstance()->C2) - 4);
    ui->CscanData_Amp->setCurrentIndex(static_cast<int>(App::getInstance()->C1));

    ui->Resolution_value->setValue(App::getInstance()->currentPosition);
    ui->Resolution_value_2->setValue(App::getInstance()->currentPosition);
    ui->spinResolution->setValue(App::getInstance()->resolutionEcoder1);
    ui->spinResolution_index->setValue(App::getInstance()->resolutionEcoder2);
    if (App::getInstance()->EncoderSwapped) {
        ui->Resolution_X->setValue(App::getInstance()->resolutionEcoder2);
    } else {
        ui->Resolution_X->setValue(App::getInstance()->resolutionEcoder1);
    }

    ui->time_resolute_value->setValue(1.0 / App::getInstance()->timeResolution);
    ui->End_value->setValue(App::getInstance()->ScanEnd);
    ui->ScanStart_value->setValue(App::getInstance()->ScanStart);
    ui->time_lengt_value->setValue(App::getInstance()->Scanlength);
    ui->SoundStart_value->setValue(App::getInstance()->SoundStart);
    ui->SoundEnd_value->setValue(App::getInstance()->SoundEnd);
    ui->End_value_2->setValue(App::getInstance()->ScanEnd);

    ui->Resolution_value_3->setValue(App::getInstance()->scanResolution);
    ui->End_value_3->setValue(App::getInstance()->End);

    ui->CscanData_Amp->blockSignals(false);
    ui->CscanData_TOF->blockSignals(false);

    ui->Resolution_value->blockSignals(false);
    ui->time_resolute_value->blockSignals(false);
    ui->spinResolution->blockSignals(false);
    ui->spinResolution_index->blockSignals(false);
    ui->Resolution_X->blockSignals(false);

    ui->End_value->blockSignals(false);
    ui->End_value_2->blockSignals(false);
    ui->time_lengt_value->blockSignals(false);
    ui->SoundStart_value->blockSignals(false);
    ui->SoundEnd_value->blockSignals(false);

    ui->Resolution_value_3->blockSignals(false);
    ui->End_value_3->blockSignals(false);
}

void Scanning::loadParameters()
{
    if (!paramManager->paramFileExists()) {
        // 没有参数文件，将当前UI控件的默认值保存到文件
        saveParameters();
        return;
    }

    isLoadingParams = true;

    QJsonObject params = paramManager->loadParams();

    // 加载CScan数据选择
    if (params.contains("Cscan_AMP")) {
        ui->CscanData_Amp->setCurrentIndex(params["Cscan_AMP"].toInt());
        App::getInstance()->C1 = static_cast<App::Cscan_Data>(params["Cscan_AMP"].toInt());
    }

    if (params.contains("Cscan_TOF")) {
        ui->CscanData_TOF->setCurrentIndex(params["Cscan_TOF"].toInt() - 4);
        App::getInstance()->C2 = static_cast<App::Cscan_Data>(params["Cscan_TOF"].toInt());
    }

    // 加载声程参数
    if (params.contains("SoundStart")) {
        ui->SoundStart_value->setValue(params["SoundStart"].toDouble());
        App::getInstance()->SoundStart = params["SoundStart"].toDouble();
    }

    if (params.contains("SoundEnd")) {
        ui->SoundEnd_value->setValue(params["SoundEnd"].toDouble());
        App::getInstance()->SoundEnd = params["SoundEnd"].toDouble();
    }

    // 加载扫描模式
    if (params.contains("EncoderIndex")) {
        ui->comboBox->setCurrentIndex(params["EncoderIndex"].toInt());
        App::getInstance()->EncoderIndex = params["EncoderIndex"].toInt();
    }

    // 加载轴方向
    if (params.contains("EncoderSwapped")) {
        bool swapped = params["EncoderSwapped"].toBool();
        App::getInstance()->EncoderSwapped = swapped;
        if (swapped) {
            ui->btnY_X->setChecked(true);
            ui->position->setText(tr("Y_Position(mm)"));
            ui->res_label->setText(tr("Y_Resolution(step/mm)"));
        } else {
            ui->btnX_Y->setChecked(true);
            ui->position->setText(tr("X_Position(mm)"));
            ui->res_label->setText(tr("X_Resolution(step/mm)"));
        }
    }

    // 加载编码器参数
    if (params.contains("ScanStart")) {
        ui->ScanStart_value->setValue(params["ScanStart"].toDouble());
        App::getInstance()->ScanStart = params["ScanStart"].toDouble();
    }

    if (params.contains("ScanEnd")) {
        ui->End_value->setValue(params["ScanEnd"].toDouble());
        ui->End_value_2->setValue(params["ScanEnd"].toDouble());
        App::getInstance()->ScanEnd = params["ScanEnd"].toDouble();
    }

    if (params.contains("currentPosition")) {
        ui->Resolution_value->setValue(params["currentPosition"].toDouble());
        ui->Resolution_value_2->setValue(params["currentPosition"].toDouble());
        App::getInstance()->currentPosition = params["currentPosition"].toDouble();
    }

    if (params.contains("resolutionEcoder1")) {
        ui->spinResolution->setValue(params["resolutionEcoder1"].toDouble());
        ui->Resolution_X->setValue(params["resolutionEcoder1"].toDouble());
        App::getInstance()->resolutionEcoder1 = params["resolutionEcoder1"].toDouble();
    }

    if (params.contains("resolutionEcoder2")) {
        ui->spinResolution_index->setValue(params["resolutionEcoder2"].toDouble());
        App::getInstance()->resolutionEcoder2 = params["resolutionEcoder2"].toDouble();
    }

    // 加载时间参数
    if (params.contains("timeResolution")) {
        double timeRes = params["timeResolution"].toDouble();
        ui->time_resolute_value->setValue(1.0 / timeRes);
        App::getInstance()->timeResolution = timeRes;
    }

    if (params.contains("Scanlength")) {
        ui->time_lengt_value->setValue(params["Scanlength"].toDouble());
        App::getInstance()->Scanlength = params["Scanlength"].toDouble();
    }

    // 加载步进轴参数
    if (params.contains("scanResolution")) {
        ui->Resolution_value_3->setValue(params["scanResolution"].toDouble());
        App::getInstance()->scanResolution = params["scanResolution"].toDouble();
    }

    if (params.contains("End")) {
        ui->End_value_3->setValue(params["End"].toDouble());
        App::getInstance()->End = params["End"].toDouble();
    }

    // 加载方向设置
    if (params.contains("xNormal")) {
        ui->xDirCombox->setCurrentIndex(params["xNormal"].toInt());
        App::getInstance()->xNormal = params["xNormal"].toInt();
    }

    if (params.contains("yNormal")) {
        ui->yDirCombox->setCurrentIndex(params["yNormal"].toInt());
        App::getInstance()->yNormal = params["yNormal"].toInt();
    }

    if (params.contains("encoderDirection")) {
        ui->EncoderComboBox->setCurrentIndex(params["encoderDirection"].toInt());
    }

    // 根据EncoderIndex设置stackedWidget
    ui->stackedWidget->setCurrentIndex(App::getInstance()->EncoderIndex);

    isLoadingParams = false;
}

void Scanning::saveParameters()
{
    if (isLoadingParams)
        return; // 正在加载时不保存

    QJsonObject params;

    // 保存CScan数据选择
    params["Cscan_AMP"] = ui->CscanData_Amp->currentIndex();
    params["Cscan_TOF"] = ui->CscanData_TOF->currentIndex() + 4;

    // 保存声程参数
    params["SoundStart"] = ui->SoundStart_value->value();
    params["SoundEnd"] = ui->SoundEnd_value->value();

    // 保存扫描模式
    params["EncoderIndex"] = ui->comboBox->currentIndex();

    // 保存轴方向
    params["EncoderSwapped"] = ui->btnY_X->isChecked();

    // 保存编码器参数（resolutionEcoder从App读取，避免不活跃tab的UI控件值为0）
    params["ScanStart"] = ui->ScanStart_value->value();
    params["ScanEnd"] = ui->End_value->value();
    params["currentPosition"] = ui->Resolution_value->value();
    params["resolutionEcoder1"] = App::getInstance()->resolutionEcoder1;
    params["resolutionEcoder2"] = App::getInstance()->resolutionEcoder2;

    // 保存时间参数
    params["timeResolution"] = 1.0 / ui->time_resolute_value->value();
    params["Scanlength"] = ui->time_lengt_value->value();

    // 保存步进轴参数
    params["scanResolution"] = ui->Resolution_value_3->value();
    params["End"] = ui->End_value_3->value();

    // 保存方向设置
    params["xNormal"] = ui->xDirCombox->currentIndex();
    params["yNormal"] = ui->yDirCombox->currentIndex();
    params["encoderDirection"] = ui->EncoderComboBox->currentIndex();

    paramManager->saveParams(params);
}

void Scanning::connectSaveSignals()
{
    // 连接所有需要保存的控件的信号
    connect(ui->CscanData_Amp, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [=](int) { saveParameters(); });

    connect(ui->CscanData_TOF, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [=](int) { saveParameters(); });

    connect(ui->SoundStart_value, &QDoubleSpinBox::editingFinished, this,
            [=]() { saveParameters(); });

    connect(ui->SoundEnd_value, &QDoubleSpinBox::editingFinished, this,
            [=]() { saveParameters(); });

    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [=](int) { saveParameters(); });

    connect(ui->btnX_Y, &QPushButton::clicked, this, [=]() { saveParameters(); });

    connect(ui->btnY_X, &QPushButton::clicked, this, [=]() { saveParameters(); });

    connect(ui->ScanStart_value, &QDoubleSpinBox::editingFinished, this,
            [=]() { saveParameters(); });

    connect(ui->End_value, &QDoubleSpinBox::editingFinished, this, [=]() { saveParameters(); });

    connect(ui->Resolution_value, &QDoubleSpinBox::editingFinished, this,
            [=]() { saveParameters(); });

    connect(ui->Resolution_value_2, &QDoubleSpinBox::editingFinished, this,
            [=]() { saveParameters(); });

    connect(ui->Resolution_X, &QDoubleSpinBox::editingFinished, this, [=]() { saveParameters(); });

    connect(ui->spinResolution, &QDoubleSpinBox::editingFinished, this,
            [=]() { saveParameters(); });

    connect(ui->spinResolution_index, &QDoubleSpinBox::editingFinished, this,
            [=]() { saveParameters(); });

    connect(ui->time_resolute_value, &QDoubleSpinBox::editingFinished, this,
            [=]() { saveParameters(); });

    connect(ui->time_lengt_value, &QDoubleSpinBox::editingFinished, this,
            [=]() { saveParameters(); });

    connect(ui->Resolution_value_3, &QDoubleSpinBox::editingFinished, this,
            [=]() { saveParameters(); });

    connect(ui->End_value_3, &QDoubleSpinBox::editingFinished, this, [=]() { saveParameters(); });

    connect(ui->xDirCombox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [=](int) { saveParameters(); });

    connect(ui->yDirCombox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [=](int) { saveParameters(); });

    connect(ui->EncoderComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [=](int) { saveParameters(); });
}

void Scanning::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
}

void Scanning::changeEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        if (ui->btnY_X->isChecked()) {
            ui->position->setText(tr("Y_Position(mm)"));
            ui->res_label->setText(tr("Y_Resolution(step/mm)"));
        } else if (ui->btnX_Y->isChecked()) {
            ui->position->setText(tr("X_Position(mm)"));
            ui->res_label->setText(tr("X_Resolution(step/mm)"));
        }
        break;
    default:
        break;
    }
    QWidget::changeEvent(event);
}

void Scanning::on_tabWidget_currentChanged(int index)
{

    if (index == 0) { // 编码器 en:Encoder
        emit App::getInstance()->signal_Reset_CScan(0);
        App::getInstance()->ScanStart = ui->ScanStart_value->value();
        App::getInstance()->ScanEnd = ui->End_value->value();
        App::getInstance()->currentPosition = ui->Resolution_value->value(); // 分辨率？
    } else if (index == 1) {
        emit App::getInstance()->signal_Reset_CScan(1);
        App::getInstance()->Scanlength = ui->time_lengt_value->value();
        App::getInstance()->timeResolution = 1.0 / ui->time_resolute_value->value();
    }
}

void Scanning::on_CscanData_Amp_currentIndexChanged(int index)
{
    App::getInstance()->C1 = static_cast<App::Cscan_Data>(index);
    // getCurrentPara();
}

void Scanning::on_CscanData_TOF_currentIndexChanged(int index)
{
    App::getInstance()->C2 = static_cast<App::Cscan_Data>(index + 4);
    // getCurrentPara();
}

void Scanning::on_Resolution_value_editingFinished()
{
    auto value = ui->Resolution_value->value();
    if (value <= 0) {
        value = 0.1;
    }
    App::getInstance()->currentPosition = value;
    double range = App::getInstance()->ScanEnd - App::getInstance()->ScanStart;
    double curPos = App::getInstance()->currentPosition;
    if (curPos > 0) {
        double end = fmod(range, curPos);
        if (value > range) {
            App::getInstance()->ScanEnd = value + App::getInstance()->ScanStart;
        } else {
            App::getInstance()->ScanEnd = App::getInstance()->ScanEnd - end;
        }
    }

    emit App::getInstance()->signal_Reset_CScan(ui->tabWidget->currentIndex());
}

void Scanning::on_End_value_editingFinished()
{

    App::getInstance()->ScanEnd = static_cast<int>(ui->End_value->value());
    emit App::getInstance()->signal_Reset_CScan(ui->tabWidget->currentIndex());
}

void Scanning::on_SoundEnd_value_editingFinished()
{
    auto arg1 = ui->SoundEnd_value->value();
    App::getInstance()->SoundEnd = arg1;
    emit App::getInstance()->signal_soundChanged();
}

void Scanning::on_SoundStart_value_editingFinished()
{
    auto arg1 = ui->SoundStart_value->value();
    App::getInstance()->SoundStart = arg1;
    emit App::getInstance()->signal_soundChanged();
}

void Scanning::on_time_resolute_value_editingFinished()
{
    double resolution = static_cast<int>(ui->time_resolute_value->value());
    App::getInstance()->timeResolution = (resolution > 0) ? (1.0 / resolution) : 1.0;
    emit App::getInstance()->signal_Reset_CScan(ui->tabWidget->currentIndex());
}

void Scanning::on_time_lengt_value_editingFinished()
{

    App::getInstance()->Scanlength = static_cast<int>(ui->time_lengt_value->value());
    emit App::getInstance()->signal_Reset_CScan(ui->tabWidget->currentIndex());
}

void Scanning::on_ScanStart_value_editingFinished()
{

    auto arg1 = ui->ScanStart_value->value();
    ui->ScanStart_value->blockSignals(true);
    ui->ScanStart_value->setValue(arg1);
    ui->ScanStart_value->blockSignals(false);
    App::getInstance()->ScanStart = arg1;
    emit App::getInstance()->signal_Reset_CScan(ui->tabWidget->currentIndex());
}

void Scanning::on_comboBox_currentIndexChanged(int index)
{
    ui->stackedWidget->setCurrentIndex(index);
    App::getInstance()->EncoderIndex = index;
    if (App::getInstance()->EncoderIndex == 0 && ui->tabWidget->currentIndex() == 1) {
        emit App::getInstance()->signal_Reset_CScan(1);
    } else {
        emit App::getInstance()->signal_Reset_CScan(0);
    }

    // getCurrentPara();
}

void Scanning::on_btnX_Y_clicked()
{
    if (ui->btnX_Y->isChecked()) {
        App::getInstance()->EncoderSwapped = false;
        ui->position->setText(tr("X_Position(mm)"));
        ui->res_label->setText(tr("X_Resolution(step/mm)"));
        // getCurrentPara();
    }
    if (App::getInstance()->EncoderIndex == 0 && ui->tabWidget->currentIndex() == 1) {
        emit App::getInstance()->signal_Reset_CScan(1);
    } else {
        emit App::getInstance()->signal_Reset_CScan(0);
    }
}

void Scanning::on_btnY_X_clicked()
{
    if (ui->btnY_X->isChecked()) {
        App::getInstance()->EncoderSwapped = true;
        ui->position->setText(tr("Y_Position(mm)"));
        ui->res_label->setText(tr("Y_Resolution(step/mm)"));
        // getCurrentPara();
    }
    if (App::getInstance()->EncoderIndex == 0 && ui->tabWidget->currentIndex() == 1) {
        emit App::getInstance()->signal_Reset_CScan(1);
    } else {
        emit App::getInstance()->signal_Reset_CScan(0);
    }
}

void Scanning::on_End_value_2_editingFinished()
{
    auto arg1 = ui->End_value_2->value();
    App::getInstance()->ScanEnd = arg1;
    emit App::getInstance()->signal_Reset_CScan(ui->tabWidget->currentIndex());
}

void Scanning::on_Resolution_value_2_editingFinished()
{
    auto value = ui->Resolution_value_2->value();
    if (value <= 0) {
        value = 0.1;
    }
    App::getInstance()->currentPosition = value;
    double range = App::getInstance()->ScanEnd - App::getInstance()->ScanStart;
    double curPos = App::getInstance()->currentPosition;
    if (curPos > 0) {
        double end = fmod(range, curPos);
        if (value > range) {
            App::getInstance()->ScanEnd = value + App::getInstance()->ScanStart;
        } else {
            App::getInstance()->ScanEnd = App::getInstance()->ScanEnd - end;
        }
    }
    emit App::getInstance()->signal_Reset_CScan(0);
}

void Scanning::on_tabWidget_2_currentChanged(int index)
{
    if (index == 0) { // 扫查轴 en:Scanning axis
        // emit App::getInstance()->signal_Reset_CScan(0);
        App::getInstance()->currentPosition = ui->Resolution_value_2->value();
        App::getInstance()->ScanEnd = ui->End_value_2->value();
        App::getInstance()->resolutionEcoder1 = ui->spinResolution->value();
    } else if (index == 1) { // 步进轴 en:Stepper shaft
        // emit App::getInstance()->signal_Reset_CScan(1);
        App::getInstance()->scanResolution = ui->Resolution_value_3->value();
        App::getInstance()->End = ui->End_value_3->value();
        App::getInstance()->resolutionEcoder2 = ui->spinResolution_index->value();
    }
}

void Scanning::on_Resolution_value_3_editingFinished()
{
    // 步进轴扫查分辨率
    auto arg1 = ui->Resolution_value_3->value();
    App::getInstance()->scanResolution = arg1;
    if (arg1 > App::getInstance()->End) {
        App::getInstance()->End = arg1;
    }
    // getCurrentPara();
}

void Scanning::on_End_value_3_editingFinished()
{
    auto arg1 = ui->End_value_3->value();
    double scanResolution = App::getInstance()->scanResolution;
    if (scanResolution <= 0)
        scanResolution = 1.0;
    double temp = arg1 / scanResolution;
    int roundedTemp = static_cast<int>(std::round(temp));
    if (arg1 < scanResolution) {
        arg1 = scanResolution;
    } else {
        arg1 = roundedTemp * scanResolution;
    }
    App::getInstance()->End = arg1;
    emit App::getInstance()->signal_Reset_CScan(ui->tabWidget->currentIndex());
    // getCurrentPara();
}

void Scanning::on_spinResolution_editingFinished()
{
    auto arg1 = ui->spinResolution->value();
    App::getInstance()->resolutionEcoder1 = arg1;
    // getCurrentPara();
}

void Scanning::on_spinResolution_index_editingFinished()
{
    auto arg1 = ui->spinResolution_index->value();
    App::getInstance()->resolutionEcoder2 = arg1;
    // getCurrentPara();
}

void Scanning::on_Resolution_X_editingFinished()
{
    auto arg1 = ui->Resolution_X->value();
    if (App::getInstance()->EncoderSwapped) {
        App::getInstance()->resolutionEcoder2 = arg1;
    } else {
        App::getInstance()->resolutionEcoder1 = arg1;
    }

    // getCurrentPara();
}

void Scanning::on_EncoderComboBox_currentIndexChanged(int index)
{

    if (ui->btnX_Y->isChecked()) {
        App::getInstance()->xNormal = index;
    } else {
        App::getInstance()->yNormal = index;
    }
    emit App::getInstance()->signal_Reset_CScan(ui->tabWidget->currentIndex());
}

void Scanning::on_xDirCombox_currentIndexChanged(int index)
{
    if (ui->btnX_Y->isChecked()) {
        App::getInstance()->xNormal = index;
        App::getInstance()->yNormal = ui->yDirCombox->currentIndex();
    } else {
        App::getInstance()->yNormal = index;
        App::getInstance()->xNormal = ui->yDirCombox->currentIndex();
    }
    emit App::getInstance()->signal_Reset_CScan(ui->tabWidget->currentIndex());
}

void Scanning::on_yDirCombox_currentIndexChanged(int index)
{

    if (ui->btnX_Y->isChecked()) {
        App::getInstance()->yNormal = index;
        App::getInstance()->xNormal = ui->xDirCombox->currentIndex();
    } else {
        App::getInstance()->xNormal = index;
        App::getInstance()->yNormal = ui->xDirCombox->currentIndex();
    }
    emit App::getInstance()->signal_Reset_CScan(ui->tabWidget->currentIndex());
}
