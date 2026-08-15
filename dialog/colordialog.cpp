#include "colordialog.h"

#include <QFileDialog>
#include <QPushButton>

#include "qdebug.h"
#include "qdom.h"
#include "ui_colordialog.h"
// #include <QScrollBar>
// #include <QListWidgetItem>

const int colorBarHeight = 256;
const int colorBarWidth = 64;

static inline QString getFilenameFromPath(const QString& file)
{
    QString name = file;
    int index = file.lastIndexOf(".");
    name.truncate(index);

    const QStringList tempSeparators = {"/", "\\", "\\\\", "\\\\\\"};
    QVector<int> position;

    for (const auto& val : tempSeparators) {
        int pos = file.lastIndexOf(val);
        if (pos >= 0)
            position << pos;
    }

    auto maxVal = *std::max_element(position.begin(), position.end());
    if (maxVal >= 0) {
        name = name.right(name.length() - maxVal - 1);
    } else {
        name = "UNKNOWN";
    }
    return name;
}

ColorDialog::ColorDialog(const QString& colorFilesPath, QWidget* parent)
    : QDialog(parent), ui(new Ui::ColorDialog), btnGroup(new QButtonGroup), filsPath(colorFilesPath)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | windowFlags());

    colorListWidget = new listWidget;

    QFont f = colorListWidget->font();
    f.setPointSize(16);
    colorListWidget->setFont(f);

    ui->listWidget_2->addWidget(colorListWidget);
    colorListWidget->verticalScrollBar()->setStyleSheet("QScrollBar{width:40px;}");
    colorListWidget->setStyleSheet("background:#ffffff");

    this->setStyleSheet("background:#D7D8DC");

    choosedFile = "";
    // findout how many Palette files
    findAllFiles(filsPath);

    // 准备一个QImage绘制调色板
    QImage image(colorBarWidth, colorBarHeight, QImage::Format_Indexed8);
    image.fill(0);
    QVector<QRgb> rgbTable0;
    for (auto i = 0; i < colorBarHeight + 4; i++) {
        rgbTable0.push_back(i | i << 8 | i << 16);
    }
    image.setColorTable(rgbTable0);

    for (auto i = 0; i < colorBarHeight; i++) {
        for (auto j = 0; j < colorBarWidth; j++)
            image.setPixel(j, i, i);
    }

    for (const auto& val : allColorfiles) {
        QVector<QColor> colorVector;
        NdtLoadColorMap(colorVector, val);
        if (colorVector.size() >= 256) {
            validColorfiles << val;
            QListWidgetItem* item = new QListWidgetItem;

            QVector<QRgb> rgbTable(colorVector.size());
            std::transform(colorVector.begin(), colorVector.end(), rgbTable.begin(), [&](QColor n) -> QRgb { return QRgb(n.rgb()); });

            QImage tempImage = image;
            tempImage.setColorTable(rgbTable);

            item->setIcon(QIcon(QPixmap::fromImage(tempImage)));
            item->setSizeHint(QSize(colorBarHeight, colorBarWidth));
            item->setText(getFilenameFromPath(val));
            colorListWidget->addItem(item);
        }
    }

    ui->OK->setStyleSheet(
        "QPushButton { background: #FFFFFF; }"
        "QPushButton:pressed { background: #E5F1FB;color:#000000; }");

    ui->Cancel->setStyleSheet(
        "QPushButton { background: #FFFFFF; }"
        "QPushButton:pressed { background: #E5F1FB;color:#000000; }");

    QObject::connect(colorListWidget, &QListWidget::currentRowChanged, this, &ColorDialog::listWidget_currentRowChanged);

    connect(ui->OK, &QPushButton::clicked, this, &QDialog::accept);  // OK → accept()
    connect(ui->Cancel, &QPushButton::clicked, this,
            &QDialog::reject);  // Cancel → reject()
}

ColorDialog::~ColorDialog()
{
    delete colorListWidget;
    delete btnGroup;
    delete ui;
}

const QString& ColorDialog::getColorFile() const
{
    return choosedFile;
}

void ColorDialog::slotButtonClicked(QAbstractButton* btn)
{
    int pos = btnGroup->id(btn);
    if (pos < 0 || pos >= validColorfiles.size()) return;
    choosedFile = validColorfiles.at(pos);
    ui->label_colorBar_2->setPixmap(btn->icon().pixmap(QSize(colorBarWidth, colorBarHeight)));
}

bool ColorDialog::NdtLoadColorMap(QVector<QColor>& colorMap, const QString& file)
{
    QFile xmlFile(file);
    if (!xmlFile.open(QIODevice::ReadOnly)) {
        return false;
    }

    QDomDocument document;
    if (!document.setContent(&xmlFile)) {
        xmlFile.close();
        return false;
    }
    xmlFile.close();

    QVector<uint32_t> SpecialColors;
    QDomElement docElem = document.documentElement();

    QDomNode n = docElem.firstChild();
    while (!n.isNull()) {
        QDomElement e = n.toElement();  // try to convert the node to an element.
        if (e.tagName() == "Version") {
        }
        if (e.tagName() == "SpecialColors") {
            // 取出SpecialColors
            if (!e.isNull()) {
                QDomNode n1 = e.firstChild();
                while (!n1.isNull()) {
                    QDomElement e1 = n1.toElement();
                    SpecialColors.push_back(QColor(e1.attribute("R").toInt(),
                                                   e1.attribute("G").toInt(),
                                                   e1.attribute("B").toInt())
                                                .rgb());
                    n1 = n1.nextSibling();
                }
            }
        }
        if (e.tagName() == "MainColors") {
            // MainColors
            if (!e.isNull()) {
                QDomNode n1 = e.firstChild();
                colorMap.clear();
                while (!n1.isNull()) {
                    QDomElement e1 = n1.toElement();
                    colorMap.push_back(QColor(e1.attribute("R").toInt(), e1.attribute("G").toInt(), e1.attribute("B").toInt()));
                    n1 = n1.nextSibling();
                }
            }
        }

        n = n.nextSibling();
    }

    colorMap.resize(colorMap.size() + SpecialColors.size());

    std::transform(SpecialColors.begin(), SpecialColors.end(), colorMap.end() - SpecialColors.size(), [&](uint32_t n) -> QColor { return QColor(n); });

    // for (const auto &val : SpecialColors) {
    //     colorMap.push_back(QColor(val));
    // }

    if (colorMap.size() >= 256)
        return true;
    else
        return false;
}

// 找出所有调色板文件
void ColorDialog::findAllFiles(const QString& path)
{
    QDir dir(path);
    QStringList filters;
    filters << "*.pal";
    filters << "*.xml";
    const QStringList files = dir.entryList(
        filters, QDir::Files | QDir::Readable | QDir::AllDirs | QDir::NoDotAndDotDot, QDir::Name);

    for (const auto& val : files) {
        QString tempPath = path + "\\" + val;
        if (val.right(4) == ".pal" || val.right(4) == ".xml") {
            allColorfiles << tempPath;
        } else {
            findAllFiles(tempPath);
        }
    }
}

void ColorDialog::listWidget_currentRowChanged(int currentRow)
{
    if (currentRow < 0 || currentRow >= validColorfiles.size()) return;
    choosedFile = validColorfiles.at(currentRow);
    if (colorListWidget->currentItem()) {
        ui->label_colorBar_2->setPixmap(
            colorListWidget->currentItem()->icon().pixmap(QSize(colorBarWidth, colorBarHeight)));
    }
}
void ColorDialog::changeEvent(QEvent* event)
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
