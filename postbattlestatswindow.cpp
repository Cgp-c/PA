#include "postbattlestatswindow.h"
#include "unitvisuals.h"

#include <QPainter>
#include <QCloseEvent>
#include <QFont>

static constexpr int PBS_W = 560;
static constexpr int PBS_H = 560;
static constexpr int PBS_ROW_H = 26;
static constexpr int PBS_HEAD_Y = 96;
static constexpr int PBS_ROWS_TOP = 122;

PostBattleStatsWindow::PostBattleStatsWindow(QWidget* parent)
    : QWidget(parent, Qt::Window)
{
    setWindowTitle(QString::fromUtf8("战斗统计 - Synera"));
    setFixedSize(PBS_W, PBS_H);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setAttribute(Qt::WA_QuitOnClose, false);
}

PostBattleStatsWindow::~PostBattleStatsWindow() = default;

void PostBattleStatsWindow::setResults(const QString& title, const QString& subtitle,
                                       const std::vector<StatRow>& rows)
{
    m_title = title;
    m_subtitle = subtitle;
    m_rows = rows;
    update();
}

void PostBattleStatsWindow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor(24, 24, 34));

    // 标题 + 副标题
    QFont titleFont;
    titleFont.setPixelSize(20);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(QColor(255, 210, 60));
    painter.drawText(QRect(20, 18, PBS_W - 40, 30),
                     Qt::AlignLeft | Qt::AlignVCenter, m_title);

    QFont subFont;
    subFont.setPixelSize(10);
    painter.setFont(subFont);
    painter.setPen(QColor(160, 160, 180));
    painter.drawText(QRect(20, 48, PBS_W - 40, 18),
                     Qt::AlignLeft | Qt::AlignVCenter, m_subtitle);

    // 表头
    QFont headFont;
    headFont.setPixelSize(10);
    headFont.setBold(true);
    painter.setFont(headFont);
    painter.setPen(QColor(120, 120, 140));

    const int nameX = 76;                      // 立绘块右侧
    const int colDealt  = PBS_W - 380;
    const int colTaken  = PBS_W - 292;
    const int colHealed = PBS_W - 204;
    const int colKills  = PBS_W - 116;

    painter.drawText(QRect(nameX, PBS_HEAD_Y, 120, 20), Qt::AlignVCenter,
                     QString::fromUtf8("英雄"));
    painter.drawText(QRect(colDealt, PBS_HEAD_Y, 84, 20), Qt::AlignVCenter,
                     QString::fromUtf8("输出伤害"));
    painter.drawText(QRect(colTaken, PBS_HEAD_Y, 84, 20), Qt::AlignVCenter,
                     QString::fromUtf8("承受伤害"));
    painter.drawText(QRect(colHealed, PBS_HEAD_Y, 84, 20), Qt::AlignVCenter,
                     QString::fromUtf8("治疗"));
    painter.drawText(QRect(colKills, PBS_HEAD_Y, 84, 20), Qt::AlignVCenter,
                     QString::fromUtf8("击杀"));

    painter.setPen(QPen(QColor(70, 70, 90), 1));
    painter.drawLine(20, PBS_HEAD_Y + 22, PBS_W - 20, PBS_HEAD_Y + 22);

    // 数据行（最多显示 12 行，超出滚动省略——人口上限 10+ 以内基本够用）
    QFont cellFont;
    cellFont.setPixelSize(11);
    painter.setFont(cellFont);

    int totalDealt = 0, totalTaken = 0, totalHealed = 0, totalKills = 0;
    const int maxRows = (PBS_H - PBS_ROWS_TOP - 70) / PBS_ROW_H;
    for (size_t i = 0; i < m_rows.size() && (int)i < maxRows; ++i) {
        const StatRow& r = m_rows[i];
        int y = PBS_ROWS_TOP + (int)i * PBS_ROW_H;

        // 斑马纹底色
        if (i % 2 == 0) {
            painter.fillRect(QRect(20, y, PBS_W - 40, PBS_ROW_H - 2), QColor(30, 30, 42));
        }

        // 小立绘块 + 名字（含星数）
        drawUnitChip(painter, QRect(24, y + 2, 22, PBS_ROW_H - 6),
                     static_cast<UnitType>(r.type), true, 3);
        painter.setPen(QColor(220, 220, 235));
        QFont nameFont;
        nameFont.setPixelSize(11);
        nameFont.setBold(true);
        painter.setFont(nameFont);
        QString starText;
        for (int s = 0; s < r.star; ++s) starText += QString::fromUtf8("★");
        painter.drawText(QRect(nameX, y, 150, PBS_ROW_H - 2), Qt::AlignVCenter,
                         r.name + (starText.isEmpty() ? QString() : " " + starText));

        painter.setFont(cellFont);
        painter.setPen(QColor(255, 150, 110));
        painter.drawText(QRect(colDealt, y, 84, PBS_ROW_H - 2), Qt::AlignVCenter,
                         QString::number(r.dealt));
        painter.setPen(QColor(180, 160, 200));
        painter.drawText(QRect(colTaken, y, 84, PBS_ROW_H - 2), Qt::AlignVCenter,
                         QString::number(r.taken));
        painter.setPen(QColor(120, 230, 140));
        painter.drawText(QRect(colHealed, y, 84, PBS_ROW_H - 2), Qt::AlignVCenter,
                         QString::number(r.healed));
        painter.setPen(QColor(255, 220, 100));
        painter.drawText(QRect(colKills, y, 84, PBS_ROW_H - 2), Qt::AlignVCenter,
                         QString::number(r.kills));

        totalDealt += r.dealt;
        totalTaken += r.taken;
        totalHealed += r.healed;
        totalKills += r.kills;
    }

    // 合计行
    int totalY = PBS_H - 52;
    painter.setPen(QPen(QColor(90, 90, 110), 1));
    painter.drawLine(20, totalY - 6, PBS_W - 20, totalY - 6);

    QFont totalFont;
    totalFont.setPixelSize(12);
    totalFont.setBold(true);
    painter.setFont(totalFont);
    painter.setPen(QColor(230, 230, 240));
    painter.drawText(QRect(nameX, totalY, 150, 22), Qt::AlignVCenter,
                     QString::fromUtf8("合计（%1 名英雄）").arg((int)m_rows.size()));
    painter.drawText(QRect(colDealt, totalY, 84, 22), Qt::AlignVCenter, QString::number(totalDealt));
    painter.drawText(QRect(colTaken, totalY, 84, 22), Qt::AlignVCenter, QString::number(totalTaken));
    painter.drawText(QRect(colHealed, totalY, 84, 22), Qt::AlignVCenter, QString::number(totalHealed));
    painter.drawText(QRect(colKills, totalY, 84, 22), Qt::AlignVCenter, QString::number(totalKills));

    painter.setPen(QColor(120, 120, 135));
    QFont hintFont;
    hintFont.setPixelSize(9);
    painter.setFont(hintFont);
    painter.drawText(QRect(20, PBS_H - 26, PBS_W - 40, 16), Qt::AlignLeft,
                     QString::fromUtf8("注：燃烧持续伤害未计入个人输出；承受含技能/燃烧/反伤全部来源"));
}

void PostBattleStatsWindow::closeEvent(QCloseEvent* event)
{
    event->ignore();
    hide();
}
