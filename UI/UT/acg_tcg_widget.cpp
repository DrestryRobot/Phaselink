#include "acg_tcg_widget.h"

#include "ui_acg_tcg_widget.h"

ACG_TCG_widget::ACG_TCG_widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ACG_TCG_widget)
    , config(Client::getInstance())
{
    ui->setupUi(this);
    initWidget();
}

ACG_TCG_widget::~ACG_TCG_widget()
{
    delete ui;
}

void ACG_TCG_widget::initWidget()
{
    ui->tableWidget->setFocusPolicy(Qt::NoFocus);
    ui->tableWidget->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    ui->beamLabel->setText(tr("Beam"));
    ui->m_beamSpinBox->setMinimumSize(0, 40);
    ui->m_beamSpinBox->setRange(1, qMax(1, config.getBeamCounts()));
    ui->m_beamSpinBox->setValue(1);
    m_currentBeam = 1;

    connect(ui->m_beamSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &ACG_TCG_widget::handleBeamSpinBoxValueChanged);
    connect(ui->tableWidget, &QTableWidget::cellChanged, this, &ACG_TCG_widget::onCellChanged);

    connect(App::getInstance(), &App::signal_tcgChanged, this, [this] {
        ui->m_beamSpinBox->setRange(1, qMax(1, config.getBeamCounts()));

        if (!config.getTcgEnable()) {
            tcg_enable = false;
            ui->TCG_ON_Btn->setChecked(false);
            ui->TCG_ON_Btn->setText(tr("TCG OFF"));
        } else {
            tcg_enable = true;
            ui->TCG_ON_Btn->setChecked(true);
            ui->TCG_ON_Btn->setText(tr("TCG ON"));
        }
        loadTcgPoints();
        refreshTabwidget();
    });

    refreshTabwidget();
}

void ACG_TCG_widget::loadTcgPoints()
{
    m_tcgPoints.clear();
    int beamCount = qMax(1, config.getBeamCounts());

    for (int b = 0; b < beamCount; b++) {
        for (int i = 1;; i++) {
            double depth = config.getTcgPointDepth(i, b);
            double gain = config.getTcgPointGain(i, b);
            if (depth < 0.0)
                break;
            m_tcgPoints.push_back({ i, b });
        }
    }
}

void ACG_TCG_widget::handleBeamSpinBoxValueChanged(int beam)
{
    m_currentBeam = beam;
    loadTcgPoints();
    refreshTabwidget();
}

void ACG_TCG_widget::onDeleteRow()
{
    QPushButton *button = qobject_cast<QPushButton *>(sender());
    if (!button)
        return;

    int row = button->property("row").toInt();
    if (row < 0 || row >= static_cast<int>(m_tcgPoints.size()))
        return;

    int index = m_tcgPoints[row].first;
    int beamIndex = m_tcgPoints[row].second;

    config.setTcgPointDepth(index, -1.0, beamIndex);

    loadTcgPoints();
    refreshTabwidget();
}

void ACG_TCG_widget::refreshTabwidget()
{
    int beamIndex = m_currentBeam - 1;

    ui->tableWidget->blockSignals(true);
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(m_tcgPoints.size());

    for (int i = 0; i < static_cast<int>(m_tcgPoints.size()); i++) {
        int index = m_tcgPoints[i].first;
        int beam = m_tcgPoints[i].second;

        auto indexItem = new QTableWidgetItem(QString::number(i + 1));
        indexItem->setTextAlignment(Qt::AlignCenter);
        indexItem->setFlags(indexItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(i, 0, indexItem);

        auto beamItem = new QTableWidgetItem(QString::number(beam + 1));
        beamItem->setTextAlignment(Qt::AlignCenter);
        beamItem->setFlags(beamItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(i, 1, beamItem);

        auto depthItem =
            new QTableWidgetItem(QString::number(config.getTcgPointDepth(index, beam)));
        depthItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget->setEditTriggers(QAbstractItemView::AllEditTriggers);
        ui->tableWidget->setItem(i, 2, depthItem);

        auto gainItem = new QTableWidgetItem(QString::number(config.getTcgPointGain(index, beam)));
        gainItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget->setItem(i, 3, gainItem);

        deleteButton = new QPushButton(tr("delete"));
        deleteButton->setProperty("row", i);
        deleteButton->setStyleSheet("font:12px;");
        connect(deleteButton, &QPushButton::clicked, this, &ACG_TCG_widget::onDeleteRow);
        ui->tableWidget->setCellWidget(i, 4, deleteButton);
    }
    ui->tableWidget->blockSignals(false);
    ui->tableWidget->scrollToBottom();
}

void ACG_TCG_widget::onCellChanged(int row, int column)
{
    if (row < 0 || row >= static_cast<int>(m_tcgPoints.size()))
        return;

    int index = m_tcgPoints[row].first;
    int beamIndex = m_tcgPoints[row].second;

    QTableWidgetItem *item = ui->tableWidget->item(row, column);
    if (!item)
        return;

    bool ok;
    double value = item->text().toDouble(&ok);
    if (!ok)
        return;

    if (column == 2) {
        config.setTcgPointDepth(index, value, beamIndex);
    } else if (column == 3) {
        config.setTcgPointGain(index, value, beamIndex);
    }

    loadTcgPoints();
    refreshTabwidget();
}

void ACG_TCG_widget::changeEvent(QEvent *event)
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

void ACG_TCG_widget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
}

void ACG_TCG_widget::on_TCG_ON_Btn_clicked()
{
    if (tcg_enable) {
        tcg_enable = false;
        config.setTcgEnable(false);
        ui->TCG_ON_Btn->setChecked(false);
        ui->TCG_ON_Btn->setText(tr("TCG OFF"));
    } else {
        tcg_enable = true;
        config.setTcgEnable(true);
        ui->TCG_ON_Btn->setChecked(true);
        ui->TCG_ON_Btn->setText(tr("TCG ON"));
    }
}

void ACG_TCG_widget::on_Add_point_Btn_clicked()
{
    int beamIndex = m_currentBeam - 1;

    int nextIndex = 1;
    for (;; nextIndex++) {
        double depth = config.getTcgPointDepth(nextIndex, beamIndex);
        if (depth < 0.0)
            break;
    }

    config.setTcgPointDepth(nextIndex, ui->TCG_pathSpinBox->value(), beamIndex);
    config.setTcgPointGain(nextIndex, ui->TCG_GainSpinBox->value(), beamIndex);

    loadTcgPoints();
    refreshTabwidget();
}

int ACG_TCG_widget::findNextAvailableIndex(const std::vector<int> &numbers)
{
    for (int nextIndex = 1;; ++nextIndex) {
        if (std::find(numbers.begin(), numbers.end(), nextIndex) == numbers.end())
            return nextIndex;
    }
    return -1;
}
