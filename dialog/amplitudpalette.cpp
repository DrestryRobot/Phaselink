#include "amplitudpalette.h"
#include "qdebug.h"
#include "qpainter.h"
#include "ui_amplitudpalette.h"
static const uint32_t color_Amplitude[] = {
    0xffffffff, 0xfffafcfe, 0xfff6fafd, 0xfff2f7fd, 0xffeef5fc, 0xffeaf2fb, 0xffe6f0fb, 0xffe1edfa,
    0xffddebf9, 0xffd9e8f9, 0xffd5e6f8, 0xffd1e3f7, 0xffcde1f7, 0xffc8def6, 0xffc4dcf6, 0xffc0d9f5,
    0xffbcd7f4, 0xffb8d4f4, 0xffb4d2f3, 0xffafd0f2, 0xffabcdf2, 0xffa7cbf1, 0xffa3c8f0, 0xff9fc6f0,
    0xff9bc3ef, 0xff96c1ef, 0xff92beee, 0xff8ebced, 0xff8ab9ed, 0xff86b7ec, 0xff82b4eb, 0xff7db2eb,
    0xff79afea, 0xff75ade9, 0xff71aae9, 0xff6da8e8, 0xff69a6e8, 0xff66a1e5, 0xff639de2, 0xff6099df,
    0xff5d95dc, 0xff5a91da, 0xff588dd7, 0xff5589d4, 0xff5285d1, 0xff4f81cf, 0xff4c7dcc, 0xff4979c9,
    0xff4775c6, 0xff4471c3, 0xff416dc1, 0xff3e69be, 0xff3b65bb, 0xff3861b8, 0xff365db6, 0xff3359b3,
    0xff3055b0, 0xff2d51ad, 0xff2a4daa, 0xff2749a8, 0xff2545a5, 0xff2241a2, 0xff1f3d9f, 0xff1c399d,
    0xff19359a, 0xff163197, 0xff142d94, 0xff112991, 0xff0e258f, 0xff0b218c, 0xff081d89, 0xff051986,
    0xff031584, 0xff041883, 0xff061c83, 0xff082083, 0xff0a2483, 0xff0c2883, 0xff0e2c83, 0xff103082,
    0xff123482, 0xff143882, 0xff153c82, 0xff174082, 0xff194482, 0xff1b4881, 0xff1d4c81, 0xff1f5081,
    0xff215481, 0xff235881, 0xff255c81, 0xff266080, 0xff286480, 0xff2a6880, 0xff2c6c80, 0xff2e7080,
    0xff307480, 0xff32787f, 0xff347c7f, 0xff36807f, 0xff37847f, 0xff39887f, 0xff3b8c7f, 0xff3d907e,
    0xff3f947e, 0xff41987e, 0xff439c7e, 0xff45a07e, 0xff47a47e, 0xff4ca67b, 0xff51a878, 0xff56aa75,
    0xff5bac72, 0xff60ae6f, 0xff65b06c, 0xff6ab269, 0xff6fb467, 0xff74b764, 0xff79b961, 0xff7ebb5e,
    0xff83bd5b, 0xff88bf58, 0xff8dc155, 0xff92c353, 0xff97c550, 0xff9cc74d, 0xffa1ca4a, 0xffa6cc47,
    0xffabce44, 0xffb0d041, 0xffb5d23f, 0xffbad43c, 0xffbfd639, 0xffc4d836, 0xffc9da33, 0xffcedd30,
    0xffd3df2d, 0xffd8e12b, 0xffdde328, 0xffe2e525, 0xffe7e722, 0xffece91f, 0xfff1eb1c, 0xfff6ed19,
    0xfffcf017, 0xfffaec19, 0xfff9e91b, 0xfff8e61d, 0xfff7e31f, 0xfff6df22, 0xfff5dc24, 0xfff4d926,
    0xfff2d628, 0xfff1d32b, 0xfff0cf2d, 0xffefcc2f, 0xffeec931, 0xffedc633, 0xffecc236, 0xffeabf38,
    0xffe9bc3a, 0xffe8b93c, 0xffe7b63f, 0xffe6b241, 0xffe5af43, 0xffe4ac45, 0xffe2a947, 0xffe1a54a,
    0xffe0a24c, 0xffdf9f4e, 0xffde9c50, 0xffdd9953, 0xffdc9555, 0xffda9257, 0xffd98f59, 0xffd88c5b,
    0xffd7885e, 0xffd68560, 0xffd58262, 0xffd47f64, 0xffd37c67, 0xffd27b64, 0xffd27b62, 0xffd27b60,
    0xffd27a5e, 0xffd17a5b, 0xffd17a59, 0xffd17957, 0xffd17955, 0xffd07953, 0xffd07850, 0xffd0784e,
    0xffd0784c, 0xffcf774a, 0xffcf7747, 0xffcf7745, 0xffcf7643, 0xffce7641, 0xffce763f, 0xffce753c,
    0xffce753a, 0xffcd7538, 0xffcd7436, 0xffcd7433, 0xffcd7431, 0xffcc732f, 0xffcc732d, 0xffcc732b,
    0xffcc7228, 0xffcb7226, 0xffcb7224, 0xffcb7122, 0xffcb711f, 0xffca711d, 0xffca701b, 0xffca7019,
    0xffca7017, 0xffc86d17, 0xffc66a17, 0xffc56717, 0xffc36417, 0xffc26217, 0xffc05f18, 0xffbe5c18,
    0xffbd5918, 0xffbb5718, 0xffba5418, 0xffb85118, 0xffb74e19, 0xffb54b19, 0xffb34919, 0xffb24619,
    0xffb04319, 0xffaf4019, 0xffad3e1a, 0xffab3b1a, 0xffaa381a, 0xffa8351a, 0xffa7321a, 0xffa5301a,
    0xffa42d1b, 0xffa22a1b, 0xffa0271b, 0xff9f251b, 0xff9d221b, 0xff9c1f1b, 0xff9a1c1c, 0xff98191c,
    0xff97171c, 0xff95141c, 0xff94111c, 0xff920e1c, 0xff910c1d, 0xff8f091d, 0xff8e061d, 0xff8c031d,
    0xff000000, 0xffffffff, 0xff808080
};
AmplitudPalette::AmplitudPalette(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AmplitudPalette)
{
    ui->setupUi(this);
    // 初始化默认颜色
    m_colors.resize(sizeof(color_Amplitude) / sizeof(uint32_t));
    for (int i = 0; i < m_colors.size(); ++i) {
        m_colors[i] = color_Amplitude[i];
    }
}

AmplitudPalette::~AmplitudPalette()
{
    delete ui;
}

void AmplitudPalette::setColors(const QVector<QRgb> &colors)
{
    if (colors.size() > 0) {
        m_colors = colors;
        update(); // 触发重绘
    }
}

const QVector<QRgb> &AmplitudPalette::getColors() const
{
    return m_colors;
}

void AmplitudPalette::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update(); // 触发 paintEvent
}

void AmplitudPalette::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    if (m_colors.isEmpty())
        return;

    QLinearGradient gradient(0, height(), 0, 0); // 从下到上的渐变

    for (int i = 0; i < m_colors.size(); ++i) {
        float pos = static_cast<float>(i) / (m_colors.size() - 1);
        gradient.setColorAt(pos, QColor::fromRgba(m_colors[i]));
    }

    painter.fillRect(rect(), gradient);
}

void AmplitudPalette::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    ColorDialog *colorDialog = new ColorDialog(QString(":/PAL"), this);

    connect(colorDialog, &QDialog::accepted, this, [this, colorDialog]() {
        QVector<QColor> colors;
        if (ColorDialog::NdtLoadColorMap(colors, colorDialog->getColorFile())) {
            QVector<QRgb> rgbs;
            rgbs.reserve(colors.size());


            for (const auto &color : qAsConst(colors)) {
                rgbs.push_back(color.rgba());
            }

            setColors(rgbs);
            emit App::getInstance()->signal_paletteChanged();
        }
    });

    connect(colorDialog, &QDialog::finished, colorDialog, &QObject::deleteLater);
    colorDialog->exec();
}
