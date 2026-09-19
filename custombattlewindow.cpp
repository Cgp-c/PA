#include "custombattlewindow.h"
#include "unitvisuals.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QCloseEvent>
#include <QFont>

// ═══════════════════════════════════════════════════════════════
// 布局常量
// ═══════════════════════════════════════════════════════════════

static constexpr int CBW_W = 560;
static constexpr int CBW_H = 600;
static constexpr int CBW_MARGIN = 20;
static constexpr int CBW_ROW_H = 42;
static constexpr int CBW_ROW_GAP = 8;
static constexpr int CBW_ROWS_TOP = 118;
static constexpr int CBW_BOTTOM_Y = CBW_H - 96;   // 底部操作区 y

// ═══════════════════════════════════════════════════════════════
// 构造 / 析构
// ═══════════════════════════════════════════════════════════════

CustomBattleWindow::CustomBattleWindow(QWidget* parent)
    : QWidget(parent, Qt::Window)
{
    setWindowTitle(QString::fromUtf8("自定义难度 - Synera"));
    setFixedSize(CBW_W, CBW_H);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setAttribute(Qt::WA_QuitOnClose, false);
    setMouseTracking(true);

    resetToDefault();
}

CustomBattleWindow::~CustomBattleWindow() = default;

void CustomBattleWindow::resetToDefault()
{
    m_specs.clear();
    m_specs.push_back({static_cast<int>(UnitType::Warrior), 0, 1});
    update();
}

// ═══════════════════════════════════════════════════════════════
// 校验
// ═══════════════════════════════════════════════════════════════

int CustomBattleWindow::totalEnemies() const
{
    int total = 0;
    for (const EnemySpec& s : m_specs)
        total += s.count;
    return total;
}

bool CustomBattleWindow::isValid() const
{
    if (m_specs.isEmpty() || m_specs.size() > MAX_ROWS) return false;
    int total = 0;
    for (const EnemySpec& s : m_specs) {
        if (s.type < 0 || s.type > static_cast<int>(UnitType::Boss)) return false;
        if (s.star < 0 || s.star > 3) return false;
        if (s.count < 1) return false;
        total += s.count;
        if (total > MAX_ENEMIES) return false;   // 提前截断，防溢出
    }
    return total > 0 && total <= MAX_ENEMIES;
}

// ═══════════════════════════════════════════════════════════════
// 渲染
// ═══════════════════════════════════════════════════════════════

QString CustomBattleWindow::typeButtonText(int type)
{
    switch (static_cast<UnitType>(type)) {
        case UnitType::Warrior:  return QString::fromUtf8("战士");
        case UnitType::Mage:     return QString::fromUtf8("法师");
        case UnitType::Support:  return QString::fromUtf8("辅助");
        case UnitType::Assassin: return QString::fromUtf8("刺客");
        case UnitType::Boss:     return QString::fromUtf8("Boss");
    }
    return "?";
}

QString CustomBattleWindow::starButtonText(int type, int star)
{
    if (static_cast<UnitType>(type) == UnitType::Boss)
        return QString::fromUtf8("—");   // Boss 属性固定，无星级
    return QString::fromUtf8("%1星").arg(star);
}

QColor CustomBattleWindow::typeButtonColor(int type)
{
    return typeFillColor(static_cast<UnitType>(type), false);
}

void CustomBattleWindow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 背景
    painter.fillRect(rect(), QColor(24, 24, 34));

    // 标题
    QFont titleFont;
    titleFont.setPixelSize(20);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(QColor(255, 210, 60));
    painter.drawText(QRect(CBW_MARGIN, 18, CBW_W - 2 * CBW_MARGIN, 28),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QString::fromUtf8("自定义难度"));

    QFont subFont;
    subFont.setPixelSize(10);
    painter.setFont(subFont);
    painter.setPen(QColor(160, 160, 180));
    painter.drawText(QRect(CBW_MARGIN, 46, CBW_W - 2 * CBW_MARGIN, 18),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QString::fromUtf8("独立于关卡的自定义对手战斗 — 点击方框切换类型 / 星级，+/- 调整数量"));

    // 列标题
    QFont headFont;
    headFont.setPixelSize(10);
    headFont.setBold(true);
    painter.setFont(headFont);
    painter.setPen(QColor(120, 120, 140));
    int x = CBW_MARGIN;
    painter.drawText(QRect(x, 88, 92, 20), Qt::AlignVCenter, QString::fromUtf8("角色类型"));
    x += 92 + 8;
    painter.drawText(QRect(x, 88, 76, 20), Qt::AlignVCenter, QString::fromUtf8("等级"));
    x += 76 + 8;
    painter.drawText(QRect(x, 88, 110, 20), Qt::AlignVCenter, QString::fromUtf8("数量"));
    x += 110 + 8;
    painter.drawText(QRect(x, 88, 60, 20), Qt::AlignVCenter, QString::fromUtf8("删除"));

    painter.setPen(QPen(QColor(70, 70, 90), 1));
    painter.drawLine(CBW_MARGIN, 110, CBW_W - CBW_MARGIN, 110);

    // 配置行
    m_typeRects.clear(); m_starRects.clear();
    m_minusRects.clear(); m_plusRects.clear(); m_delRects.clear();

    QFont btnFont;
    btnFont.setPixelSize(11);
    btnFont.setBold(true);

    for (int i = 0; i < m_specs.size() && i < MAX_ROWS; ++i) {
        const EnemySpec& spec = m_specs[i];
        int y = CBW_ROWS_TOP + i * (CBW_ROW_H + CBW_ROW_GAP);
        if (y + CBW_ROW_H > CBW_BOTTOM_Y - 8) break;

        int bx = CBW_MARGIN;

        // 类型按钮
        QRect typeRect(bx, y, 92, CBW_ROW_H);
        m_typeRects.push_back(typeRect);
        bx += typeRect.width() + 8;
        painter.setBrush(typeButtonColor(spec.type));
        painter.setPen(QPen(QColor(240, 240, 240), 1));
        painter.drawRoundedRect(typeRect, 5, 5);
        painter.setFont(btnFont);
        painter.setPen(Qt::white);
        painter.drawText(typeRect, Qt::AlignCenter, typeButtonText(spec.type));

        // 星级按钮
        QRect starRect(bx, y, 76, CBW_ROW_H);
        m_starRects.push_back(starRect);
        bx += starRect.width() + 8;
        bool isBoss = static_cast<UnitType>(spec.type) == UnitType::Boss;
        painter.setBrush(isBoss ? QColor(50, 50, 60) : QColor(45, 55, 90));
        painter.setPen(QPen(isBoss ? QColor(90, 90, 100) : QColor(110, 140, 220), 1));
        painter.drawRoundedRect(starRect, 5, 5);
        painter.setPen(isBoss ? QColor(130, 130, 140) : QColor(180, 200, 255));
        painter.drawText(starRect, Qt::AlignCenter, starButtonText(spec.type, spec.star));

        // 数量 -/值/+（合一行的三段热区）
        QRect numRect(bx, y, 110, CBW_ROW_H);
        bx += numRect.width() + 8;
        painter.setBrush(QColor(38, 38, 50));
        painter.setPen(QPen(QColor(80, 80, 100), 1));
        painter.drawRoundedRect(numRect, 5, 5);
        QRect minusRect = numRect.adjusted(4, 6, -numRect.width() + 32, -6);
        QRect plusRect  = numRect.adjusted(numRect.width() - 28, 6, -4, -6);
        m_minusRects.push_back(minusRect);
        m_plusRects.push_back(plusRect);
        painter.setPen(spec.count > 1 ? QColor(230, 200, 80) : QColor(110, 110, 120));
        painter.setFont(btnFont);
        painter.drawText(minusRect, Qt::AlignCenter, "-");
        painter.setPen(totalEnemies() < MAX_ENEMIES ? QColor(230, 200, 80) : QColor(110, 110, 120));
        painter.drawText(plusRect, Qt::AlignCenter, "+");
        painter.setPen(Qt::white);
        QFont cntFont;
        cntFont.setPixelSize(14);
        cntFont.setBold(true);
        painter.setFont(cntFont);
        painter.drawText(numRect, Qt::AlignCenter, QString("x%1").arg(spec.count));

        // 删除按钮
        QRect delRect(bx, y, 60, CBW_ROW_H);
        m_delRects.push_back(delRect);
        painter.setBrush(QColor(90, 40, 40));
        painter.setPen(QPen(QColor(200, 100, 100), 1));
        painter.drawRoundedRect(delRect, 5, 5);
        painter.setFont(btnFont);
        painter.setPen(QColor(255, 180, 180));
        painter.drawText(delRect, Qt::AlignCenter, QString::fromUtf8("删除"));
    }

    // 添加一组按钮（行区末尾）
    int addY = CBW_ROWS_TOP + m_specs.size() * (CBW_ROW_H + CBW_ROW_GAP);
    m_addRect = QRect(CBW_MARGIN, addY, 92 + 8 + 76 + 8 + 110 + 8 + 60, 30);
    bool canAdd = m_specs.size() < MAX_ROWS && addY + 30 < CBW_BOTTOM_Y;
    painter.setBrush(canAdd ? QColor(40, 70, 50) : QColor(45, 45, 50));
    painter.setPen(QPen(canAdd ? QColor(90, 190, 110) : QColor(90, 90, 95), 1));
    painter.drawRoundedRect(m_addRect, 5, 5);
    painter.setFont(btnFont);
    painter.setPen(canAdd ? QColor(150, 240, 170) : QColor(120, 120, 125));
    painter.drawText(m_addRect, Qt::AlignCenter, QString::fromUtf8("+ 添加一组敌方"));

    // ── 底部操作区 ──
    painter.setPen(QPen(QColor(70, 70, 90), 1));
    painter.drawLine(CBW_MARGIN, CBW_BOTTOM_Y, CBW_W - CBW_MARGIN, CBW_BOTTOM_Y);

    int total = totalEnemies();
    bool valid = isValid();

    // 总数计数与校验提示
    QFont totalFont;
    totalFont.setPixelSize(13);
    totalFont.setBold(true);
    painter.setFont(totalFont);
    QColor totalColor = (total > MAX_ENEMIES) ? QColor(255, 90, 90)
                       : (total == 0)         ? QColor(200, 160, 60)
                                              : QColor(90, 230, 110);
    painter.setPen(totalColor);
    painter.drawText(QRect(CBW_MARGIN, CBW_BOTTOM_Y + 10, 260, 22),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QString::fromUtf8("敌方总数: %1 / %2").arg(total).arg(MAX_ENEMIES));

    QFont hintFont;
    hintFont.setPixelSize(9);
    painter.setFont(hintFont);
    painter.setPen(QColor(150, 150, 165));
    QString hint;
    if (total > MAX_ENEMIES)
        hint = QString::fromUtf8("超过战场容量上限，请减少数量");
    else if (total == 0)
        hint = QString::fromUtf8("至少需要 1 个敌方单位");
    else
        hint = QString::fromUtf8("开始后以当前棋盘上的英雄迎战，胜负不影响关卡进度与玩家 HP");
    painter.drawText(QRect(CBW_MARGIN, CBW_BOTTOM_Y + 34, CBW_W - 2 * CBW_MARGIN, 16),
                     Qt::AlignLeft | Qt::AlignVCenter, hint);

    // 清空按钮
    m_clearRect = QRect(CBW_W - CBW_MARGIN - 320, CBW_BOTTOM_Y + 14, 100, 34);
    painter.setBrush(QColor(70, 50, 40));
    painter.setPen(QPen(QColor(200, 140, 90), 1));
    painter.drawRoundedRect(m_clearRect, 5, 5);
    painter.setFont(btnFont);
    painter.setPen(QColor(255, 200, 160));
    painter.drawText(m_clearRect, Qt::AlignCenter, QString::fromUtf8("清空重置"));

    // 开始按钮
    m_startRect = QRect(CBW_W - CBW_MARGIN - 200, CBW_BOTTOM_Y + 14, 200, 34);
    painter.setBrush(valid ? QColor(40, 110, 60) : QColor(55, 60, 55));
    painter.setPen(QPen(valid ? QColor(110, 230, 140) : QColor(95, 100, 95), 1));
    painter.drawRoundedRect(m_startRect, 5, 5);
    QFont startFont;
    startFont.setPixelSize(13);
    startFont.setBold(true);
    painter.setFont(startFont);
    painter.setPen(valid ? QColor(220, 255, 230) : QColor(140, 145, 140));
    painter.drawText(m_startRect, Qt::AlignCenter,
                     QString::fromUtf8("开始自定义战斗"));
}

// ═══════════════════════════════════════════════════════════════
// 交互
// ═══════════════════════════════════════════════════════════════

void CustomBattleWindow::mousePressEvent(QMouseEvent* event)
{
    const QPoint pos = event->pos();
    const int n = static_cast<int>(m_specs.size());

    for (int i = 0; i < n && i < MAX_ROWS; ++i) {
        EnemySpec& spec = m_specs[i];

        // 类型循环：战士→法师→辅助→刺客→Boss→战士
        if (i < m_typeRects.size() && m_typeRects[i].contains(pos)) {
            spec.type = (spec.type + 1) % (static_cast<int>(UnitType::Boss) + 1);
            if (static_cast<UnitType>(spec.type) == UnitType::Boss)
                spec.star = 0;   // Boss 无星级
            update();
            return;
        }

        // 星级循环 0→3（Boss 忽略）
        if (i < m_starRects.size() && m_starRects[i].contains(pos)) {
            if (static_cast<UnitType>(spec.type) != UnitType::Boss)
                spec.star = (spec.star + 1) % 4;
            update();
            return;
        }

        // 数量增减（全局总量不超上限）
        if (i < m_minusRects.size() && m_minusRects[i].contains(pos)) {
            if (spec.count > 1) --spec.count;
            update();
            return;
        }
        if (i < m_plusRects.size() && m_plusRects[i].contains(pos)) {
            if (totalEnemies() < MAX_ENEMIES) ++spec.count;
            update();
            return;
        }

        // 删除该组
        if (i < m_delRects.size() && m_delRects[i].contains(pos)) {
            m_specs.erase(m_specs.begin() + i);
            update();
            return;
        }
    }

    if (m_addRect.contains(pos)) {
        if (m_specs.size() < MAX_ROWS
            && CBW_ROWS_TOP + (m_specs.size() + 1) * (CBW_ROW_H + CBW_ROW_GAP) + 30 < CBW_BOTTOM_Y) {
            m_specs.push_back({static_cast<int>(UnitType::Mage), 0, 1});
            update();
        }
        return;
    }

    if (m_clearRect.contains(pos)) {
        resetToDefault();
        return;
    }

    if (m_startRect.contains(pos)) {
        if (isValid())
            emit startRequested();
        return;
    }
}

void CustomBattleWindow::closeEvent(QCloseEvent* event)
{
    event->ignore();
    hide();
}
