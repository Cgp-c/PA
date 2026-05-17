#include "synera.h"
#include "ui_synera.h"
#include "hero.h"
#include "enemy.h"
#include "weapon.h"

#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFont>
#include <cstdlib>
#include <algorithm>

// ═══════════════════════════════════════════════════════════════
// 构造 / 析构
// ═══════════════════════════════════════════════════════════════

Synera::Synera(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_phase(GamePhase::Placement)
    , m_gameOver(false)
    , m_combatTickCounter(0)
    , m_draggedUnit(nullptr)
    , m_dragFromPoolIndex(-1)
{
    ui->setupUi(this);
    setWindowTitle("Synera - Auto Chess Arena");
    resize(1200, 800);
    setMouseTracking(true);

    initGame();

    m_gameTimer = new QTimer(this);
    connect(m_gameTimer, &QTimer::timeout, this, &Synera::gameLoop);
    m_gameTimer->start(16);
    m_frameClock.start();
}

Synera::~Synera() { delete ui; }

// ═══════════════════════════════════════════════════════════════
// 初始化
// ═══════════════════════════════════════════════════════════════

void Synera::initGame()
{
    m_units.clear();
    m_weapons.clear();
    m_unitPool.clear();
    m_board.clear();
    m_draggedUnit = nullptr;
    m_dragFromPoolIndex = -1;
    m_phase = GamePhase::Placement;
    m_gameOver = false;
    m_combatTickCounter = 0;

    generateUnitPool();
}

void Synera::generateUnitPool()
{
    m_unitPool.clear();
    // 随机生成：战士2~3个，法师1~2个，辅助1~2个
    int wc = 2 + std::rand() % 2; // 2-3
    int mc = 1 + std::rand() % 2; // 1-2
    int sc = 1 + std::rand() % 2; // 1-2
    m_unitPool.push_back({UnitType::Warrior, wc});
    m_unitPool.push_back({UnitType::Mage,    mc});
    m_unitPool.push_back({UnitType::Support, sc});
}

Unit* Synera::createUnitFromPool(UnitType type, bool isHero)
{
    if (isHero) {
        switch (type) {
            case UnitType::Warrior: { auto u = std::make_unique<WarriorHero>(); Unit* p = u.get(); m_units.push_back(std::move(u)); return p; }
            case UnitType::Mage:    { auto u = std::make_unique<MageHero>();    Unit* p = u.get(); m_units.push_back(std::move(u)); return p; }
            case UnitType::Support: { auto u = std::make_unique<SupportHero>(); Unit* p = u.get(); m_units.push_back(std::move(u)); return p; }
        }
    } else {
        switch (type) {
            case UnitType::Warrior: { auto u = std::make_unique<WarriorEnemy>(); Unit* p = u.get(); m_units.push_back(std::move(u)); return p; }
            case UnitType::Mage:    { auto u = std::make_unique<MageEnemy>();    Unit* p = u.get(); m_units.push_back(std::move(u)); return p; }
            case UnitType::Support: { auto u = std::make_unique<SupportEnemy>(); Unit* p = u.get(); m_units.push_back(std::move(u)); return p; }
        }
    }
    return nullptr;
}

void Synera::startBattle()
{
    // 敌方随机生成 1战士 + 1法师 + 1辅助
    UnitType enemyTypes[] = {UnitType::Warrior, UnitType::Mage, UnitType::Support};
    for (int i = 0; i < 3; ++i) {
        Unit* eu = createUnitFromPool(enemyTypes[i], false);
        // 在敌方半场 (row 0-3) 随机空位放置
        bool placed = false;
        for (int attempt = 0; attempt < 100; ++attempt) {
            int ex = std::rand() % Board::SIZE;
            int ey = std::rand() % (Board::SIZE / 2);
            if (m_board.placeUnit(eu, ex, ey)) { placed = true; break; }
        }
        if (!placed) {
            for (int y = 0; y < Board::SIZE / 2 && !placed; ++y)
                for (int x = 0; x < Board::SIZE && !placed; ++x)
                    if (m_board.placeUnit(eu, x, y)) placed = true;
        }
    }

    m_phase = GamePhase::Battle;
}

// ═══════════════════════════════════════════════════════════════
// 游戏主循环
// ═══════════════════════════════════════════════════════════════

void Synera::gameLoop()
{
    float dt = m_frameClock.elapsed() / 1000.0f;
    m_frameClock.restart();
    Q_UNUSED(dt);

    if (m_phase == GamePhase::Battle && !m_gameOver) {
        ++m_combatTickCounter;
        if (m_combatTickCounter >= TICK_INTERVAL) {
            m_combatTickCounter = 0;
            processCombatTick();
        }
    }

    update();
}

// ═══════════════════════════════════════════════════════════════
// 绘制入口
// ═══════════════════════════════════════════════════════════════

void Synera::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor(25, 25, 35));

    renderBoard(painter);
    renderUnits(painter);
    renderBenchPool(painter);
    renderDragGhost(painter);
    renderUI(painter);
}

// ═══════════════════════════════════════════════════════════════
// 棋盘
// ═══════════════════════════════════════════════════════════════

void Synera::renderBoard(QPainter& painter)
{
    for (int y = 0; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            QRect rc = cellRect(x, y);

            QColor cellColor = m_board.isPlayerHalf(y)
                ? QColor(40, 55, 80) : QColor(75, 38, 38);

            // 拖拽悬停高亮
            if (m_draggedUnit && m_phase == GamePhase::Placement && m_board.isPlayerHalf(y)) {
                QRect unitRect(m_dragCurrentPos.x() - CELL_SIZE / 2,
                               m_dragCurrentPos.y() - CELL_SIZE / 2,
                               CELL_SIZE, CELL_SIZE);
                QRect inter = rc.intersected(unitRect);
                if (inter.width() * inter.height() > (CELL_SIZE * CELL_SIZE) / 2
                    && !m_board.isOccupied(x, y))
                    cellColor = QColor(50, 90, 50);
            }

            painter.fillRect(rc, cellColor);
            painter.setPen(QPen(QColor(75, 75, 95), 1));
            painter.drawRect(rc);
        }
    }
}

// ═══════════════════════════════════════════════════════════════
// 棋盘上的单位
// ═══════════════════════════════════════════════════════════════

static QColor typeFillColor(UnitType t, bool isHero)
{
    switch (t) {
        case UnitType::Warrior: return isHero ? QColor(210, 100, 30)  : QColor(180, 70, 20);
        case UnitType::Mage:    return isHero ? QColor(130, 80, 210)  : QColor(100, 55, 170);
        case UnitType::Support: return isHero ? QColor(55, 170, 100)  : QColor(40, 140, 70);
    }
    return QColor(128, 128, 128);
}

static QString typeLabel(UnitType t)
{
    switch (t) {
        case UnitType::Warrior: return QString::fromUtf8("\346\210\230"); // 战
        case UnitType::Mage:    return QString::fromUtf8("\346\263\225"); // 法
        case UnitType::Support: return QString::fromUtf8("\350\276\205"); // 辅
    }
    return "?";
}

void Synera::renderUnits(QPainter& painter)
{
    for (int y = 0; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            Unit* unit = m_board.getUnitAt(x, y);
            if (!unit || unit->isDisappeared()) continue;
            if (unit == m_draggedUnit) continue;

            QRect rc = cellRect(x, y);
            int m = 6;
            QRect ur = rc.adjusted(m, m, -m, -m);

            bool isHero = dynamic_cast<Hero*>(unit) != nullptr;
            UnitType t = unit->getType();
            QColor fill = typeFillColor(t, isHero);
            QColor border = isHero ? QColor(100, 170, 255) : QColor(235, 90, 90);

            painter.setBrush(fill);
            painter.setPen(QPen(border, 1));
            painter.drawRoundedRect(ur, 6, 6);

            // 角色标签（战/法/辅）
            painter.setPen(Qt::white);
            QFont typeFont;
            typeFont.setPixelSize(22);
            typeFont.setBold(true);
            painter.setFont(typeFont);
            painter.drawText(ur, Qt::AlignCenter, typeLabel(t));

            // 名字（顶部）
            QFont nameFont;
            nameFont.setPixelSize(10);
            nameFont.setBold(true);
            painter.setFont(nameFont);
            painter.drawText(ur.adjusted(3, 2, 0, 0),
                             QString::fromStdString(unit->getName()));

            // HP 条（底部）
            int barH = 7;
            int barY = ur.bottom() - barH - 2;
            double ratio = (double)unit->getHp() / unit->getMaxHp();
            QRect barBg(ur.left() + 2, barY, ur.width() - 4, barH);
            painter.setBrush(QColor(30, 30, 30));
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(barBg, 2, 2);

            QColor hpC = ratio > 0.5 ? QColor(80, 210, 80)
                       : ratio > 0.25 ? QColor(210, 190, 30)
                       : QColor(210, 55, 55);
            QRect barFill(barBg.left(), barY, (int)(barBg.width() * ratio), barH);
            painter.setBrush(hpC);
            painter.drawRoundedRect(barFill, 2, 2);

            QFont hpFont;
            hpFont.setPixelSize(8);
            painter.setFont(hpFont);
            painter.setPen(Qt::white);
            painter.drawText(barBg, Qt::AlignCenter,
                             QString("%1/%2").arg(unit->getHp()).arg(unit->getMaxHp()));

            // 法力值圆点
            int dotR = 4;
            int dotSpacing = 10;
            int dotY = barBg.top() - dotR - 3;
            int totalDotW = Unit::MAX_MANA * dotSpacing - (dotSpacing - dotR * 2);
            int dotStartX = ur.center().x() - totalDotW / 2;
            for (int iDot = 0; iDot < Unit::MAX_MANA; ++iDot) {
                int cx = dotStartX + iDot * dotSpacing;
                bool filled = (iDot < unit->getMana());
                painter.setBrush(filled ? QColor(240, 240, 240) : QColor(60, 60, 60));
                painter.setPen(QPen(QColor(150, 150, 150), 1));
                painter.drawEllipse(QPoint(cx, dotY), dotR, dotR);
            }

            // 装备
            if (unit->getEquipment()) {
                painter.setPen(QColor(255, 210, 0));
                QFont eqFont;
                eqFont.setPixelSize(8);
                painter.setFont(eqFont);
                painter.drawText(ur.adjusted(3, 0, 0, 0), Qt::AlignBottom | Qt::AlignLeft,
                                 QString("[%1]").arg(QString::fromStdString(unit->getEquipment()->getName())));
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════
// 左侧备战池
// ═══════════════════════════════════════════════════════════════

QRect Synera::poolSlotRect(int index) const
{
    return QRect(POOL_X, POOL_Y + index * (POOL_SLOT_H + 10), POOL_WIDTH, POOL_SLOT_H);
}

int Synera::findPoolSlotAt(const QPoint& pixel) const
{
    for (int i = 0; i < (int)m_unitPool.size(); ++i)
        if (poolSlotRect(i).contains(pixel)) return i;
    return -1;
}

void Synera::renderBenchPool(QPainter& painter)
{
    if (m_phase != GamePhase::Placement) return;

    // 标题
    QFont titleFont;
    titleFont.setPixelSize(13);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(QColor(180, 180, 200));
    painter.drawText(POOL_X, POOL_Y - 12, "Unit Pool");

    for (int i = 0; i < (int)m_unitPool.size(); ++i) {
        QRect rc = poolSlotRect(i);
        const PoolSlot& slot = m_unitPool[i];

        // 底色
        painter.setBrush(QColor(40, 40, 50));
        painter.setPen(QPen(slot.count > 0 ? QColor(110, 110, 130) : QColor(60, 60, 70), 1));
        painter.drawRoundedRect(rc, 6, 6);

        if (slot.count <= 0) {
            painter.setPen(QColor(100, 100, 100));
            QFont emptyFont;
            emptyFont.setPixelSize(12);
            painter.setFont(emptyFont);
            painter.drawText(rc, Qt::AlignCenter, "Empty");
            continue;
        }

        // 角色色块
        QRect iconRect = rc.adjusted(12, 18, -12, -40);
        QColor fill = typeFillColor(slot.type, true);
        painter.setBrush(fill);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(iconRect, 6, 6);

        // 标签
        painter.setPen(Qt::white);
        QFont iconFont;
        iconFont.setPixelSize(26);
        iconFont.setBold(true);
        painter.setFont(iconFont);
        painter.drawText(iconRect, Qt::AlignCenter, typeLabel(slot.type));

        // 名称 + 数量
        QFont infoFont;
        infoFont.setPixelSize(12);
        infoFont.setBold(true);
        painter.setFont(infoFont);

        const char* names[] = {"Warrior", "Mage", "Support"};
        painter.setPen(QColor(200, 200, 220));
        QRect nameRect = rc.adjusted(8, rc.height() - 32, -8, -8);
        painter.drawText(nameRect, Qt::AlignHCenter | Qt::AlignTop,
                         QString("%1  x%2").arg(names[(int)slot.type]).arg(slot.count));

        // 数值
        QFont statFont;
        statFont.setPixelSize(10);
        painter.setFont(statFont);
        painter.setPen(QColor(160, 160, 180));
        QString stats;
        switch (slot.type) {
            case UnitType::Warrior: stats = "HP:100 ATK:20 Rng:1"; break;
            case UnitType::Mage:    stats = "HP:50  ATK:10 Rng:4"; break;
            case UnitType::Support: stats = "HP:80  Heal:20 Rng:1"; break;
        }
        painter.drawText(nameRect.adjusted(0, 16, 0, 0), Qt::AlignHCenter | Qt::AlignTop, stats);
    }
}

// ═══════════════════════════════════════════════════════════════
// 拖拽幽灵
// ═══════════════════════════════════════════════════════════════

void Synera::renderDragGhost(QPainter& painter)
{
    if (!m_draggedUnit) return;

    QRect unitRect(m_dragCurrentPos.x() - CELL_SIZE / 2,
                   m_dragCurrentPos.y() - CELL_SIZE / 2,
                   CELL_SIZE, CELL_SIZE);

    painter.save();
    painter.setOpacity(0.75);

    bool isHero = dynamic_cast<Hero*>(m_draggedUnit) != nullptr;
    UnitType t = m_draggedUnit->getType();
    painter.setBrush(typeFillColor(t, isHero));
    painter.setPen(QPen(QColor(255, 255, 100), 2));
    painter.drawRoundedRect(unitRect, 8, 8);

    painter.setPen(Qt::white);
    QFont f;
    f.setPixelSize(20);
    f.setBold(true);
    painter.setFont(f);
    painter.drawText(unitRect, Qt::AlignCenter, typeLabel(t));

    painter.restore();
}

// ═══════════════════════════════════════════════════════════════
// 右侧 UI
// ═══════════════════════════════════════════════════════════════

void Synera::renderUI(QPainter& painter)
{
    int textX = BOARD_OFFSET_X + BOARD_PIXEL_SIZE + 30;

    // 阶段标题
    QFont uiFont;
    uiFont.setPixelSize(16);
    uiFont.setBold(true);
    painter.setFont(uiFont);

    QString status;
    if (m_gameOver) {
        status = "GAME OVER";
        painter.setPen(QColor(255, 150, 50));
    } else if (m_phase == GamePhase::Placement) {
        status = "PLACEMENT PHASE";
        painter.setPen(QColor(255, 200, 100));
    } else {
        status = "BATTLE IN PROGRESS";
        painter.setPen(QColor(255, 120, 80));
    }
    painter.drawText(textX, BOARD_OFFSET_Y + 30, status);

    // 开始战斗按钮
    if (m_phase == GamePhase::Placement) {
        int btnW = 170, btnH = 40;
        int btnX = textX, btnY = BOARD_OFFSET_Y + 55;
        m_startButtonRect = QRect(btnX, btnY, btnW, btnH);

        bool hasBoardHero = false;
        for (int y = Board::SIZE / 2; y < Board::SIZE; ++y)
            for (int x = 0; x < Board::SIZE; ++x) {
                Unit* u = m_board.getUnitAt(x, y);
                if (u && dynamic_cast<Hero*>(u)) { hasBoardHero = true; break; }
            }

        QColor btnC = hasBoardHero ? QColor(55, 150, 55) : QColor(75, 75, 75);
        painter.setBrush(btnC);
        painter.setPen(QPen(hasBoardHero ? QColor(90, 210, 90) : QColor(110, 110, 110), 2));
        painter.drawRoundedRect(m_startButtonRect, 8, 8);

        painter.setPen(Qt::white);
        QFont btnFont;
        btnFont.setPixelSize(15);
        btnFont.setBold(true);
        painter.setFont(btnFont);
        painter.drawText(m_startButtonRect, Qt::AlignCenter, "Start Battle");

        if (!hasBoardHero) {
            painter.setPen(QColor(200, 140, 40));
            QFont hintFont;
            hintFont.setPixelSize(11);
            painter.setFont(hintFont);
            painter.drawText(textX, btnY + btnH + 18, "Drag units to the board");
        }
    }

    // 图例
    int legendY = m_phase == GamePhase::Placement
        ? m_startButtonRect.bottom() + 55 : BOARD_OFFSET_Y + 75;
    QFont legFont;
    legFont.setPixelSize(12);
    painter.setFont(legFont);
    painter.setPen(QColor(170, 170, 190));
    painter.drawText(textX, legendY, "Legend:");

    struct { QString label; QColor color; } legend[] = {
        {QString::fromUtf8("\342\227\217 Hero Warrior"), typeFillColor(UnitType::Warrior, true)},
        {QString::fromUtf8("\342\227\217 Hero Mage"),    typeFillColor(UnitType::Mage, true)},
        {QString::fromUtf8("\342\227\217 Hero Support"), typeFillColor(UnitType::Support, true)},
        {QString::fromUtf8("\342\227\217 Enemy Warrior"), typeFillColor(UnitType::Warrior, false)},
        {QString::fromUtf8("\342\227\217 Enemy Mage"),    typeFillColor(UnitType::Mage, false)},
        {QString::fromUtf8("\342\227\217 Enemy Support"), typeFillColor(UnitType::Support, false)},
    };
    for (int i = 0; i < 6; ++i) {
        painter.setPen(legend[i].color);
        painter.drawText(textX + 8, legendY + 20 + i * 20, legend[i].label);
    }

    // 存活单位
    int unitListY = legendY + 20 + 6 * 20 + 20;
    painter.setPen(QColor(190, 190, 210));
    QFont listTitleFont;
    listTitleFont.setPixelSize(13);
    listTitleFont.setBold(true);
    painter.setFont(listTitleFont);
    painter.drawText(textX, unitListY, "Alive Units:");

    unitListY += 22;
    QFont listFont;
    listFont.setPixelSize(11);
    painter.setFont(listFont);
    for (auto& u : m_units) {
        if (u->isDisappeared() || u->isDead()) continue;
        bool isH = dynamic_cast<Hero*>(u.get()) != nullptr;
        painter.setPen(isH ? QColor(100, 170, 255) : QColor(240, 100, 100));
        QString info = QString("%1  HP:%2/%3  MP:%4/%5  (%6,%7)")
            .arg(QString::fromStdString(u->getName()))
            .arg(u->getHp()).arg(u->getMaxHp())
            .arg(u->getMana()).arg(u->getMaxMana())
            .arg(u->getPosition().x).arg(u->getPosition().y);
        painter.drawText(textX + 8, unitListY, info);
        unitListY += 18;
    }

    // 帮助
    int helpY = unitListY + 16;
    painter.setPen(QColor(140, 140, 160));
    QFont helpFont;
    helpFont.setPixelSize(11);
    painter.setFont(helpFont);
    if (m_phase == GamePhase::Placement) {
        painter.drawText(textX, helpY, "Drag from left pool to board");
        painter.drawText(textX, helpY + 18, "Press R to reset");
    } else {
        painter.drawText(textX, helpY, "Auto-combat in progress...");
        painter.drawText(textX, helpY + 18, "Press R to reset");
    }
}

// ═══════════════════════════════════════════════════════════════
// 坐标映射
// ═══════════════════════════════════════════════════════════════

QRect Synera::cellRect(int x, int y) const
{
    return QRect(BOARD_OFFSET_X + x * CELL_SIZE,
                 BOARD_OFFSET_Y + y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
}

Unit* Synera::findUnitAt(int x, int y) const { return m_board.getUnitAt(x, y); }

Unit* Synera::findUnitAtPixel(const QPoint& p) const
{
    int gx = (p.x() - BOARD_OFFSET_X) / CELL_SIZE;
    int gy = (p.y() - BOARD_OFFSET_Y) / CELL_SIZE;
    if (!m_board.isValidPosition(gx, gy)) return nullptr;
    return m_board.getUnitAt(gx, gy);
}

// ═══════════════════════════════════════════════════════════════
// 鼠标 — 分发
// ═══════════════════════════════════════════════════════════════

void Synera::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;
    if (m_gameOver) return;
    QPoint pos = event->pos();

    if (m_phase == GamePhase::Placement) {
        if (m_startButtonRect.contains(pos)) {
            bool hasBoardHero = false;
            for (int y = Board::SIZE / 2; y < Board::SIZE; ++y)
                for (int x = 0; x < Board::SIZE; ++x) {
                    Unit* u = m_board.getUnitAt(x, y);
                    if (u && dynamic_cast<Hero*>(u)) { hasBoardHero = true; break; }
                }
            if (hasBoardHero) startBattle();
            return;
        }
        processDragStart(pos);
    }
}

void Synera::mouseMoveEvent(QMouseEvent *event)
{
    if (m_draggedUnit) {
        m_dragCurrentPos = event->pos();
        update();
    }
    QMainWindow::mouseMoveEvent(event);
}

void Synera::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;
    if (m_draggedUnit) processDrop(event->pos());
    QMainWindow::mouseReleaseEvent(event);
}

// ═══════════════════════════════════════════════════════════════
// 拖拽
// ═══════════════════════════════════════════════════════════════

void Synera::processDragStart(const QPoint& mousePos)
{
    // 优先检查左侧备战池
    int poolIdx = findPoolSlotAt(mousePos);
    if (poolIdx >= 0 && m_unitPool[poolIdx].count > 0) {
        Unit* u = createUnitFromPool(m_unitPool[poolIdx].type, true);
        m_draggedUnit = u;
        m_dragFromPoolIndex = poolIdx;
        m_dragCurrentPos = mousePos;
        m_unitPool[poolIdx].count--;
        return;
    }

    // 检查棋盘上的英雄（可拖回池子）
    Unit* clicked = findUnitAtPixel(mousePos);
    if (clicked && dynamic_cast<Hero*>(clicked) && !clicked->isDisappeared()) {
        m_draggedUnit = clicked;
        m_dragFromPoolIndex = -1;
        m_dragCurrentPos = mousePos;
        m_board.removeUnit(clicked->getPosition().x, clicked->getPosition().y);
    }
}

void Synera::processDrop(const QPoint& mousePos)
{
    if (!m_draggedUnit) return;

    int unitArea = CELL_SIZE * CELL_SIZE;
    QRect unitRect(mousePos.x() - CELL_SIZE / 2,
                   mousePos.y() - CELL_SIZE / 2, CELL_SIZE, CELL_SIZE);

    // 放回池子？
    int poolIdx = findPoolSlotAt(mousePos);
    if (poolIdx >= 0) {
        // 返还数量
        if (m_dragFromPoolIndex >= 0)
            m_unitPool[m_dragFromPoolIndex].count++;
        else {
            // 从棋盘拖回：找到对应类型的 pool slot
            UnitType t = m_draggedUnit->getType();
            for (auto& slot : m_unitPool) {
                if (slot.type == t) { slot.count++; break; }
            }
            m_draggedUnit->setDisappeared(true); // 标记移除以避免渲染
        }
        m_draggedUnit = nullptr;
        m_dragFromPoolIndex = -1;
        update();
        return;
    }

    // 找棋盘上 >50% 重叠的空格
    int bestGx = -1, bestGy = -1, bestOverlap = 0;
    for (int y = Board::SIZE / 2; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            QRect cr = cellRect(x, y);
            QRect inter = unitRect.intersected(cr);
            if (inter.isEmpty()) continue;
            int overlap = inter.width() * inter.height();
            if (overlap > unitArea / 2 && overlap > bestOverlap && !m_board.isOccupied(x, y)) {
                bestOverlap = overlap;
                bestGx = x; bestGy = y;
            }
        }
    }

    if (bestGx >= 0) {
        if (m_dragFromPoolIndex < 0) {
            // 从棋盘拖到另一格
        }
        m_board.placeUnit(m_draggedUnit, bestGx, bestGy);
        m_dragFromPoolIndex = -1;
    } else {
        // 没合法位置：返还
        if (m_dragFromPoolIndex >= 0) {
            m_unitPool[m_dragFromPoolIndex].count++;
            m_draggedUnit->setDisappeared(true);
        } else {
            m_board.placeUnit(m_draggedUnit, m_draggedUnit->getPosition().x,
                              m_draggedUnit->getPosition().y);
        }
    }

    m_draggedUnit = nullptr;
    m_dragFromPoolIndex = -1;
    update();
}

// ═══════════════════════════════════════════════════════════════
// 自动战斗 — 单回合 tick
// ═══════════════════════════════════════════════════════════════

void Synera::processBurningTick(std::vector<Unit*>& alive)
{
    for (Unit* u : alive) {
        if (u->isBurning()) {
            u->tickBurning();
            if (u->isDead())
                m_board.removeUnit(u->getPosition().x, u->getPosition().y);
        }
    }
}

void Synera::processCombatTick()
{
    struct Move { Unit* unit; Position to; };
    std::vector<Move> moves;

    // 收集棋盘上所有存活单位
    std::vector<Unit*> alive;
    for (int y = 0; y < Board::SIZE; ++y)
        for (int x = 0; x < Board::SIZE; ++x) {
            Unit* u = m_board.getUnitAt(x, y);
            if (u && !u->isDead() && !u->isDisappeared())
                alive.push_back(u);
        }

    // 先处理燃烧伤害
    processBurningTick(alive);

    for (Unit* u : alive) {
        if (u->isDead() || u->isDisappeared()) continue;
        Position pos = u->getPosition();

        // ── 法力值满 → 释放技能 ──
        if (u->getMana() >= Unit::MAX_MANA) {
            u->useSkill(m_board, alive);
            u->resetMana();
            continue;
        }

        if (u->canHeal()) {
            // ── 辅助逻辑 ──
            Unit* healTarget = findHealTarget(u);
            if (healTarget && manhattanDist(pos, healTarget->getPosition()) <= u->getAttackRange()) {
                healTarget->heal(u->getHealAmount());
                u->gainMana();
                continue;
            }
            if (healTarget) {
                Position next = moveStepToward(pos, healTarget->getPosition());
                if (!(next == pos)) moves.push_back({u, next});
                continue;
            }
            // 无受伤队友 → 按战士逻辑移动
            Unit* enemy = findNearestEnemyFor(u);
            if (enemy) {
                Position next = moveStepToward(pos, enemy->getPosition());
                if (!(next == pos)) moves.push_back({u, next});
            }
        } else {
            // ── 战士 / 法师 ──
            Unit* target = findNearestEnemyFor(u);
            if (!target) continue;

            if (canAttack(u, target)) {
                u->attack(*target);
                u->gainMana();
                if (target->isDead())
                    m_board.removeUnit(target->getPosition().x, target->getPosition().y);
            } else {
                Position next = moveStepToward(pos, target->getPosition());
                if (!(next == pos)) moves.push_back({u, next});
            }
        }
    }

    // 执行移动
    for (auto& m : moves) {
        if (m_board.isOccupied(m.to.x, m.to.y)) continue;
        Position old = m.unit->getPosition();
        m_board.removeUnit(old.x, old.y);
        m_board.placeUnit(m.unit, m.to.x, m.to.y);
    }

    checkWinCondition();
}

// ═══════════════════════════════════════════════════════════════
// 索敌（自动战斗）
// ═══════════════════════════════════════════════════════════════

Unit* Synera::findNearestEnemyFor(Unit* unit) const
{
    bool isHero = dynamic_cast<Hero*>(unit) != nullptr;
    Unit* best = nullptr;
    int bestDist = 999;

    for (int y = 0; y < Board::SIZE; ++y)
        for (int x = 0; x < Board::SIZE; ++x) {
            Unit* u = m_board.getUnitAt(x, y);
            if (!u || u == unit || u->isDead() || u->isDisappeared()) continue;
            bool uIsHero = dynamic_cast<Hero*>(u) != nullptr;
            if (uIsHero == isHero) continue; // 同阵营

            int d = manhattanDist(unit->getPosition(), u->getPosition());
            if (d < bestDist) { bestDist = d; best = u; }
        }
    return best;
}

Unit* Synera::findHealTarget(Unit* support) const
{
    bool isHero = dynamic_cast<Hero*>(support) != nullptr;
    Unit* best = nullptr;
    int bestDist = 999;
    UnitType bestType = UnitType::Support;
    int bestXDist = 999;

    for (int y = 0; y < Board::SIZE; ++y)
        for (int x = 0; x < Board::SIZE; ++x) {
            Unit* u = m_board.getUnitAt(x, y);
            if (!u || u == support || u->isDead() || u->isDisappeared()) continue;
            if (u->getHp() >= u->getMaxHp()) continue; // 没受伤
            bool uIsHero = dynamic_cast<Hero*>(u) != nullptr;
            if (uIsHero != isHero) continue;

            int d = manhattanDist(support->getPosition(), u->getPosition());
            int xd = std::abs(support->getPosition().x - u->getPosition().x);

            bool better = false;
            if (d < bestDist) better = true;
            else if (d == bestDist) {
                int tr = (u->getType() == UnitType::Warrior) ? 0 :
                         (u->getType() == UnitType::Mage) ? 1 : 2;
                int br = (bestType == UnitType::Warrior) ? 0 :
                         (bestType == UnitType::Mage) ? 1 : 2;
                if (tr < br) better = true;
                else if (tr == br && xd < bestXDist) better = true;
            }

            if (better) {
                best = u; bestDist = d; bestType = u->getType(); bestXDist = xd;
            }
        }
    return best;
}

Position Synera::moveStepToward(const Position& from, const Position& to) const
{
    int dx = (to.x > from.x) ? 1 : (to.x < from.x) ? -1 : 0;
    int dy = (to.y > from.y) ? 1 : (to.y < from.y) ? -1 : 0;

    if (dx != 0) {
        Position next(from.x + dx, from.y);
        if (m_board.isValidPosition(next.x, next.y) && !m_board.isOccupied(next.x, next.y))
            return next;
    }
    if (dy != 0) {
        Position next(from.x, from.y + dy);
        if (m_board.isValidPosition(next.x, next.y) && !m_board.isOccupied(next.x, next.y))
            return next;
    }
    return from;
}

bool Synera::canAttack(Unit* attacker, Unit* target) const
{
    return manhattanDist(attacker->getPosition(), target->getPosition())
           <= attacker->getAttackRange();
}

// ═══════════════════════════════════════════════════════════════
// 胜负判定
// ═══════════════════════════════════════════════════════════════

void Synera::checkWinCondition()
{
    bool hasHeroNonSupport = false, hasEnemyNonSupport = false;
    bool hasHeroAny = false, hasEnemyAny = false;

    for (auto& u : m_units) {
        if (u->isDisappeared() || u->isDead()) continue;
        bool isH = dynamic_cast<Hero*>(u.get()) != nullptr;
        if (isH) {
            hasHeroAny = true;
            if (u->getType() != UnitType::Support) hasHeroNonSupport = true;
        } else {
            hasEnemyAny = true;
            if (u->getType() != UnitType::Support) hasEnemyNonSupport = true;
        }
    }

    // 一方全员阵亡，或一方只剩辅助 → 判负
    if (!hasHeroAny || !hasHeroNonSupport) m_gameOver = true;
    if (!hasEnemyAny || !hasEnemyNonSupport) m_gameOver = true;
}

// ═══════════════════════════════════════════════════════════════
// 键盘
// ═══════════════════════════════════════════════════════════════

void Synera::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_R) {
        initGame();
    }
    QMainWindow::keyPressEvent(event);
}
