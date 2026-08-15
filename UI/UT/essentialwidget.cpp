#include "essentialwidget.h"

#include <app.h>

#include <QDebug>
#include <QListView>

#include "ui_essentialwidget.h"

std::unordered_map<int, MaxAmplitude> indexToAmplitude = { { 0, MaxAmplitude::kPercentage1600 },
                                                           { 1, MaxAmplitude::kPercentage800 },
                                                           { 2, MaxAmplitude::kPercentage400 },
                                                           { 3, MaxAmplitude::kPercentage200 } };

std::unordered_map<MaxAmplitude, int> amplitudeToIndex = { { MaxAmplitude::kPercentage1600, 0 },
                                                           { MaxAmplitude::kPercentage800, 1 },
                                                           { MaxAmplitude::kPercentage400, 2 },
                                                           { MaxAmplitude::kPercentage200, 3 } };

static QVector<int> SamplePoint { 4096, 1024, 512, 256 };

static VideoFilter indexToVideoFilter[] = { VideoFilter::k1MHz, VideoFilter::k2MHz,
                                            VideoFilter::k3MHz, VideoFilter::k4MHz,
                                            VideoFilter::k5MHz, VideoFilter::k6MHz,
                                            VideoFilter::k7MHz, VideoFilter::kBypass };
static constexpr int videoFilterCount = sizeof(indexToVideoFilter) / sizeof(indexToVideoFilter[0]);

static int videoFilterToIndex(VideoFilter type)
{
    for (int i = 0; i < videoFilterCount; i++) {
        if (indexToVideoFilter[i] == type)
            return i;
    }
    return videoFilterCount - 1; // None
}

EssentialWidget::EssentialWidget(QWidget *parent)
    : NdtBase(parent)
    , ui(new Ui::EssentialWidget)
    , config(Client::getInstance())
{
    ui->setupUi(this);
    initWidget();
    initSlot();
}

EssentialWidget::~EssentialWidget()
{
    delete ui;
}

void EssentialWidget::setshowmodel(NdtBase::ShowModel mode)
{
    setShowModel(mode);
}

void EssentialWidget::initSlot()
{
    connect(App::getInstance(), &App::refresh_Allpara, this, &EssentialWidget::getcurrentPara);
}

void EssentialWidget::initWidget()
{
    ui->PolarityBox->setView(new QListView());
    ui->RectifierBox->setView(new QListView);
    ui->MamplitudeBox->setView(new QListView);
    ui->FilterHighBox->setView(new QListView);
    ui->FilterLowBox->setView(new QListView);
    ui->vedioFilterBox->setView(new QListView);

    ui->spinPRF->setStyleSheet("QSpinBox { border: none; }");
    ui->spinPRF->setReadOnly(true);
    ui->VoltageSpinBox->setDecimals(0);

    ui->GainSpinBox->installEventFilter(this);
    ui->RangstartSpinBox->installEventFilter(this);
    ui->RangendSpinBox->installEventFilter(this);
    ui->MvelocitySpinBox->installEventFilter(this);
    ui->DoffsetSpinBox->installEventFilter(this);
    ui->DvelocitySpinBox->installEventFilter(this);
    ui->Pitch_value->installEventFilter(this);
    ui->PwidthSpinBox->installEventFilter(this);
    ui->ZeroPosition->installEventFilter(this);
    ui->wedgeXSpinBox->installEventFilter(this);
    ui->PRFSpinBox->installEventFilter(this);
    ui->VoltageSpinBox->installEventFilter(this);
    ui->FrequenceSpinBox->installEventFilter(this);
    ui->SampleBox->installEventFilter(this);
    ui->wedgeAngle->installEventFilter(this);
}

void EssentialWidget::getcurrentPara()
{
    ui->GainSpinBox->blockSignals(true);
    ui->RangstartSpinBox->blockSignals(true);
    ui->RangendSpinBox->blockSignals(true);
    ui->MvelocitySpinBox->blockSignals(true);
    ui->DvelocitySpinBox->blockSignals(true);
    ui->wedgeXSpinBox->blockSignals(true);
    ui->DoffsetSpinBox->blockSignals(true);
    ui->PRFSpinBox->blockSignals(true);
    ui->VoltageSpinBox->blockSignals(true);
    ui->PwidthSpinBox->blockSignals(true);
    ui->PolarityBox->blockSignals(true);
    ui->FilterHighBox->blockSignals(true);
    ui->FilterLowBox->blockSignals(true);
    ui->RectifierBox->blockSignals(true);
    ui->MamplitudeBox->blockSignals(true);
    ui->SampleBox->blockSignals(true);
    ui->Pitch_value->blockSignals(true);
    ui->ZeroPosition->blockSignals(true);
    ui->spinPRF->blockSignals(true);
    ui->vedioFilterBox->blockSignals(true);
    ui->FrequenceSpinBox->blockSignals(true);
    ui->wedgeAngle->blockSignals(true);

    ui->GainSpinBox->setValue(static_cast<double>(config.getGain()));
    ui->MvelocitySpinBox->setValue(static_cast<double>(config.getWorkpieceVelocity()));
    ui->RangstartSpinBox->setValue(static_cast<double>(config.getRangeStart()));
    ui->RangendSpinBox->setValue(static_cast<double>(config.getRangeEnd()));

    ui->DvelocitySpinBox->setValue(static_cast<double>(config.getWedgeVelocity()));
    ui->wedgeXSpinBox->setValue(static_cast<double>(config.getWedgeX()));  // 延迟声速没有
    ui->DoffsetSpinBox->setValue(static_cast<double>(config.getWedgeZ())); // 延迟偏移量没有
    ui->PRFSpinBox->setValue(static_cast<double>(config.getFrameRate()));
    ui->VoltageSpinBox->setValue(config.getPaVoltage());
    ui->PwidthSpinBox->setValue(config.getPulseWidth());
    ui->Pitch_value->setValue(static_cast<double>(config.getProbePrimaryElementsPitch()));
    ui->ZeroPosition->setValue(static_cast<int>(config.getTfmRecDelay()));

    ui->PolarityBox->setCurrentIndex(static_cast<int>(config.getVoltagePolarity()));
    ui->vedioFilterBox->setCurrentIndex(videoFilterToIndex(config.getVideoFilterMHz()));
    if (config.getFilterHigh() == -1) {
        ui->FilterHighBox->setCurrentText(0);
    } else {
        ui->FilterHighBox->setCurrentText(QString::number(config.getFilterHigh()));
    }
    if (config.getFilterLow() == -1) {
        ui->FilterLowBox->setCurrentText(0);
    } else {
        ui->FilterLowBox->setCurrentText(QString::number(config.getFilterLow()));
    }

    ui->FrequenceSpinBox->setValue(config.getProbeFrequency());

    ui->RectifierBox->setCurrentIndex(static_cast<int>(config.getRectifierMode()));

    ui->MamplitudeBox->setCurrentIndex(amplitudeToIndex[config.getMaxAmplitude()]);

    ui->SampleBox->setValue(config.getPointQuantity());

    ui->spinPRF->setValue(config.getFrameRate() * config.getBeamCounts());

    ui->wedgeAngle->setValue(config.getWedgeAngle());

    ui->GainSpinBox->blockSignals(false);
    ui->RangstartSpinBox->blockSignals(false);
    ui->RangendSpinBox->blockSignals(false);
    ui->MvelocitySpinBox->blockSignals(false);
    ui->DvelocitySpinBox->blockSignals(false);
    ui->wedgeXSpinBox->blockSignals(false);
    ui->DoffsetSpinBox->blockSignals(false);
    ui->PRFSpinBox->blockSignals(false);
    ui->VoltageSpinBox->blockSignals(false);
    ui->PwidthSpinBox->blockSignals(false);

    ui->PolarityBox->blockSignals(false);
    ui->FilterHighBox->blockSignals(false);
    ui->FilterLowBox->blockSignals(false);
    ui->RectifierBox->blockSignals(false);
    ui->MamplitudeBox->blockSignals(false);
    ui->SampleBox->blockSignals(false);
    ui->Pitch_value->blockSignals(false);
    ui->ZeroPosition->blockSignals(false);
    ui->spinPRF->blockSignals(false);
    ui->vedioFilterBox->blockSignals(false);
    ui->FrequenceSpinBox->blockSignals(false);
    ui->wedgeAngle->blockSignals(false);
}

void EssentialWidget::changeEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
    QWidget::changeEvent(event);
}

void EssentialWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
}

bool EssentialWidget::eventFilter(QObject *watched, QEvent *event)
{
    QList<QDoubleSpinBox *> doubleSpinBoxes = findChildren<QDoubleSpinBox *>();
    for (QDoubleSpinBox *spinBox : doubleSpinBoxes) {
        if (spinBox) {
            if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
                isBlockSignal[spinBox] = true;
            } else if (event->type() == QEvent::FocusOut
                       || event->type() == QEvent::MouseButtonPress) {
                isBlockSignal[spinBox] = false;
            } else if (event->type() == QEvent::Wheel) {
                isBlockSignal[spinBox] = true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void EssentialWidget::on_PolarityBox_currentIndexChanged(int index)
{
    config.setVoltagePolarity(static_cast<PolarityType>(index));
}
void EssentialWidget::on_FilterHighBox_currentIndexChanged(int index)
{
    if (index == 0) {
        config.setFilterHigh(-1);
    }
    config.setFilterHigh(ui->FilterHighBox->currentText().toFloat());
}

void EssentialWidget::on_FilterLowBox_currentIndexChanged(int index)
{
    if (index == 0) {
        config.setFilterHigh(-1);
    }
    config.setFilterLow(ui->FilterLowBox->currentText().toFloat());
}

void EssentialWidget::on_vedioFilterBox_currentIndexChanged(int index)
{
    if (index >= 0 && index < videoFilterCount) {
        config.setVideoFilterMHz(indexToVideoFilter[index]);
    }
    emit App::getInstance()->refresh_Allpara();
}

void EssentialWidget::on_RectifierBox_currentIndexChanged(int index)
{
    config.setRectifierMode(static_cast<RectifierType>(index));
    emit App::getInstance()->signal_RectifierChanged();
    emit App::getInstance()->refresh_Allpara();
    emit App::getInstance()->signal_showhideGatestatus();
}

void EssentialWidget::on_MamplitudeBox_currentIndexChanged(int index)
{
    config.setMaxAmplitude(indexToAmplitude[ui->MamplitudeBox->currentIndex()]);
    emit App::getInstance()->signal_AmplitudeChanged();
    emit App::getInstance()->refresh_Allpara();
}

void EssentialWidget::on_SampleBox_editingFinished()
{
    config.setPointQuantity(ui->SampleBox->value());
    emit App::getInstance()->signal_RangeChanged();
    emit App::getInstance()->refresh_Allpara();
}

void EssentialWidget::on_GainSpinBox_editingFinished()
{
    config.setGain(ui->GainSpinBox->value());
}

void EssentialWidget::on_RangstartSpinBox_editingFinished()
{
    config.setRangeStart(ui->RangstartSpinBox->value());
    getcurrentPara();
    emit App::getInstance()->signal_RangeChanged();
}

void EssentialWidget::on_RangendSpinBox_editingFinished()
{
    config.setRangeEnd(ui->RangendSpinBox->value());
    emit App::getInstance()->signal_RangeChanged();
    getcurrentPara();
}

void EssentialWidget::on_wedgeXSpinBox_editingFinished()
{
    config.setWedgeX(ui->wedgeXSpinBox->value());
}

void EssentialWidget::on_DoffsetSpinBox_editingFinished()
{
    config.setWedgeZ(ui->DoffsetSpinBox->value());
}

void EssentialWidget::on_PRFSpinBox_editingFinished()
{
    auto rate = static_cast<int32_t>(ui->PRFSpinBox->value());
    config.setFrameRate(rate);
    ui->spinPRF->setValue(rate * config.getBeamCounts());
}

void EssentialWidget::on_VoltageSpinBox_editingFinished()
{
    config.setPaVoltage(static_cast<int32_t>(ui->VoltageSpinBox->value()));
}

void EssentialWidget::on_PwidthSpinBox_editingFinished()
{
    config.setPulseWidth(static_cast<int32_t>(ui->PwidthSpinBox->value()));
}

void EssentialWidget::on_MvelocitySpinBox_editingFinished()
{
    config.setWorkpieceVelocity(ui->MvelocitySpinBox->value());
    ui->RangendSpinBox->setValue(static_cast<double>(config.getRangeEnd()));
}

void EssentialWidget::on_DvelocitySpinBox_editingFinished()
{
    config.setWedgeVelocity(ui->DvelocitySpinBox->value());
}

void EssentialWidget::on_Pitch_value_editingFinished()
{
    config.setProbePrimaryElementsPitch(ui->Pitch_value->value());
    emit App::getInstance()->signal_probeChange();
}

void EssentialWidget::on_ZeroPosition_editingFinished()
{
    config.setTfmRecDelay(ui->ZeroPosition->value());
}

void EssentialWidget::on_GainSpinBox_valueChanged(double gain)
{
    if (!isBlockSignal[ui->GainSpinBox]) {
        config.setGain(gain);
    }
}

void EssentialWidget::on_RangstartSpinBox_valueChanged(double start)
{
    if (!isBlockSignal[ui->RangstartSpinBox]) {
        config.setRangeStart(start);
        getcurrentPara();
        emit App::getInstance()->signal_RangeChanged();
    }
}

void EssentialWidget::on_RangendSpinBox_valueChanged(double end)
{
    if (!isBlockSignal[ui->RangendSpinBox]) {
        config.setRangeEnd(end);
        getcurrentPara();
        emit App::getInstance()->signal_RangeChanged();
    }
}

void EssentialWidget::on_MvelocitySpinBox_valueChanged(double arg1)
{
    if (!isBlockSignal[ui->MvelocitySpinBox]) {
        config.setWorkpieceVelocity(arg1);
    }
}

void EssentialWidget::on_DvelocitySpinBox_valueChanged(double arg1)
{
    if (!isBlockSignal[ui->DvelocitySpinBox]) {
        config.setWedgeVelocity(arg1);
    }
}

void EssentialWidget::on_Pitch_value_valueChanged(double arg1)
{
    if (!isBlockSignal[ui->Pitch_value]) {
        config.setProbePrimaryElementsPitch(arg1);
        emit App::getInstance()->signal_probeChange();
    }
}

void EssentialWidget::on_wedgeXSpinBox_valueChanged(double arg1)
{
    if (!isBlockSignal[ui->wedgeXSpinBox]) {
        config.setWedgeX(arg1);
    }
}

void EssentialWidget::on_DoffsetSpinBox_valueChanged(double arg1)
{
    if (!isBlockSignal[ui->DoffsetSpinBox]) {
        config.setWedgeZ(arg1);
    }
}

void EssentialWidget::on_PRFSpinBox_valueChanged(double prf)
{
    if (!isBlockSignal[ui->PRFSpinBox]) {
        config.setFrameRate(static_cast<int32_t>(prf));
    }
}

void EssentialWidget::on_VoltageSpinBox_valueChanged(double voltage)
{
    if (!isBlockSignal[ui->VoltageSpinBox]) {
        config.setPaVoltage(static_cast<int>(voltage));
    }
}

void EssentialWidget::on_PwidthSpinBox_valueChanged(double width)
{
    if (!isBlockSignal[ui->PwidthSpinBox]) {
        config.setPulseWidth(width);
    }
}

void EssentialWidget::on_ZeroPosition_valueChanged(double position)
{
    if (!isBlockSignal[ui->ZeroPosition]) {
        config.setTfmRecDelay(static_cast<int>(position));
    }
}

void EssentialWidget::on_FrequenceSpinBox_editingFinished()
{
    config.setProbeFrequency(ui->FrequenceSpinBox->value());
}

void EssentialWidget::on_FrequenceSpinBox_valueChanged(double arg1)
{
    if (!isBlockSignal[ui->FrequenceSpinBox]) {
        config.setProbeFrequency(arg1);
    }
}

void EssentialWidget::on_SampleBox_valueChanged(double arg1)
{
    if (!isBlockSignal[ui->FrequenceSpinBox]) {
        config.setPointQuantity(arg1);
        emit App::getInstance()->signal_RangeChanged();
    }
}

void EssentialWidget::on_wedgeAngle_editingFinished()
{
    config.setWedgeAngle(ui->wedgeAngle->value());
}

void EssentialWidget::on_wedgeAngle_valueChanged(double arg1)
{
    if (!isBlockSignal[ui->wedgeAngle]) {
        config.setWedgeAngle(ui->wedgeAngle->value());
    }
}
