#include "startscreen.h"

#include <QPainter>
#include <QMouseEvent>
#include <QCloseEvent>
#include <QFile>
#include <QFont>

static const char* ENDLESS_BEST_FILE = "endless_best.txt";

StartScreen::StartScreen(QWidget* parent)
    : QWidget(parent, Qt::Window)
{
    setWindowTitle(QString::fromUtf8("Synera - 开始"));
    setFixedSize(520, 430);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setAttribute(Qt::WA_QuitOnClose, false);
    refreshBestWave();
}

StartScreen::~StartScreen() = default;

int StartScreen::loadBestWave()
{
    QFile f(ENDLESS_BEST_FILE);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return 0;
    bool ok = false;
    int v = QString::fromUtf8(f.readAll()).trimmed().toInt(&ok);
    return ok ? v : 0;
}

void StartScreen::refreshBestWave()
{
    m_bestEndlessWave = loadBestWave();
    update();
}

void StartScreen::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor(24, 24, 34));

    // 标题
    QFont titleFont;
    titleFont.setPixelSize(30);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(QColor(255, 210, 60));
    painter.drawText(QRect(20, 36, width() - 40, 44),
                     Qt::AlignHCenter, "Synera");

    QFont subFont;
    subFont.setPixelSize(12);
    painter.setFont(subFont);
    painter.setPen(QColor(160, 160, 180));
    painter.drawText(QRect(20, 82, width() - 40, 20),
                     Qt::AlignHCenter, QString::fromUtf8("自动战斗自走棋 — 选择游戏模式"));

    struct ModeInfo {
        int mode;
        const char* name;
        const char* desc;
        QColor fill, border, text;
    };
    const ModeInfo modes[3] = {
        { MODE_CAMPAIGN, "\347\200\217\345\215\241\346\250\241\345\274\217",
                          "5 \345\205\263\351\200\222\350\277\233\357\274\214\346\211\223\350\264\245 Boss \345\215\263\350\203\234\345\210\251",
                          QColor(40, 70, 50),  QColor(110, 230, 140), QColor(220, 255, 230) },
        { MODE_ENDLESS, "\346\227\240\347\251\267\346\250\241\345\274\217",
                         "\346\263\242\346\254\241\346\227\240\351\231\220\357\274\214\346\200\252\347\211\251\351\200\220\345\242\233\357\274\214\347\234\213\350\203\275\346\222\211\345\210\260\347\254\254\345\207\240\346\263\242",
                         QColor(70, 45, 80),  QColor(190, 110, 230), QColor(230, 200, 250) },
        { MODE_CUSTOM, "\350\207\252\345\256\232\344\271\211\346\250\241\345\274\217",
                        "\350\207\252\347\224\261\351\205\215\347\275\256\345\257\271\346\211\213\347\274\226\346\210\220\347\232\204\346\262\231\347\256\261",
                        QColor(45, 60, 100), QColor(110, 150, 230), QColor(200, 220, 255) },
    };

    m_modeRects.clear();
    for (int i = 0; i < 3; ++i) {
        int y = 130 + i * 82;
        QRect rc(60, y, width() - 120, 66);
        m_modeRects.push_back(rc);

        painter.setBrush(modes[i].fill);
        painter.setPen(QPen(modes[i].border, 2));
        painter.drawRoundedRect(rc, 10, 10);

        QFont nameFont;
        nameFont.setPixelSize(17);
        nameFont.setBold(true);
        painter.setFont(nameFont);
        painter.setPen(modes[i].text);
        painter.drawText(QRect(rc.left() + 18, rc.top(), 130, rc.height()),
                         Qt::AlignVCenter, QString::fromUtf8(modes[i].name));

        QFont descFont;
        descFont.setPixelSize(10);
        painter.setFont(descFont);
        painter.setPen(QColor(190, 190, 205));
        painter.drawText(QRect(rc.left() + 150, rc.top(), rc.width() - 165, rc.height()),
                         Qt::AlignVCenter | Qt::TextWordWrap, QString::fromUtf8(modes[i].desc));
    }

    // 底部信息：无尽最佳 + 操作提示
    QFont footFont;
    footFont.setPixelSize(10);
    painter.setFont(footFont);
    painter.setPen(QColor(140, 140, 160));
    QString best = m_bestEndlessWave > 0
        ? QString::fromUtf8("无尽模式最佳纪录：第 %1 波").arg(m_bestEndlessWave)
        : QString::fromUtf8("无尽模式：暂无纪录");
    painter.drawText(QRect(20, height() - 52, width() - 40, 18),
                     Qt::AlignHCenter, best);
    painter.drawText(QRect(20, height() - 32, width() - 40, 18),
                     Qt::AlignHCenter,
                     QString::fromUtf8("游戏中按 R 返回模式选择"));
}

void StartScreen::mousePressEvent(QMouseEvent* event)
{
    const QPoint pos = event->pos();
    for (int i = 0; i < (int)m_modeRects.size(); ++i) {
        if (m_modeRects[i].contains(pos)) {
            hide();
            emit modeSelected(i);
            return;
        }
    }
}

void StartScreen::closeEvent(QCloseEvent* event)
{
    event->ignore();
    hide();
}
