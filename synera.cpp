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
    , m_phase(GamePhase::Preparation)
    , m_gameOver(false)
    , m_playerVictory(false)
    , m_showLevelLoss(false)
    , m_frameCounter(0)
    , m_currentLevel(1)
    , m_playerHp(100)
    , m_gold(280)
    , m_pendingGold(0)
    , m_recycleSlots(8, nullptr)
    , m_draggedUnit(nullptr)
    , m_dragFromShopIndex(-1)
    , m_dragFromRecycleIndex(-1)
{
    ui->setupUi(this);
    setWindowTitle("Synera - Auto Chess Arena");
    resize(960, 780);
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
    m_shop.clear();
    m_board.clear();
    for (auto& p : m_recycleSlots) p = nullptr;
    m_draggedUnit = nullptr;
    m_dragFromShopIndex = -1;
    m_dragFromRecycleIndex = -1;
    m_phase = GamePhase::Preparation;
    m_gameOver = false;
    m_playerVictory = false;
    m_showLevelLoss = false;
    m_frameCounter = 0;
    m_currentLevel = 1;
    m_playerHp = 100;
    m_gold = 280;
    m_pendingGold = 0;

    // 初始化商店：4 种类型，初始库存 0
    m_shop.clear();
    m_shop.push_back({UnitType::Warrior, 0});
    m_shop.push_back({UnitType::Mage, 0});
    m_shop.push_back({UnitType::Support, 0});
    m_shop.push_back({UnitType::Assassin, 0});

    m_buyButtonRects.clear();
}

void Synera::initLevel()
{
    // 清理已死亡/消失的单位
    m_units.erase(
        std::remove_if(m_units.begin(), m_units.end(),
            [](const std::unique_ptr<Unit>& u) { return u->isDisappeared() || u->isDead(); }),
        m_units.end());

    m_board.clear();
    m_draggedUnit = nullptr;
    m_dragFromShopIndex = -1;
    m_dragFromRecycleIndex = -1;
    m_phase = GamePhase::Preparation;
    m_frameCounter = 0;
    m_pendingGold = 0;

    // 回收槽中已死亡的清理
    for (auto& p : m_recycleSlots) {
        if (p && (p->isDead() || p->isDisappeared()))
            p = nullptr;
    }
}

// ═══════════════════════════════════════════════════════════════
// 金币 / 价格
// ═══════════════════════════════════════════════════════════════

int Synera::heroCost(UnitType t) const
{
    switch (t) {
        case UnitType::Warrior:  return 100;
        case UnitType::Mage:     return 80;
        case UnitType::Support:  return 50;
        case UnitType::Assassin: return 60;
    }
    return 0;
}

int Synera::enemyGoldValue(UnitType t) const
{
    switch (t) {
        case UnitType::Warrior:  return 80;
        case UnitType::Mage:     return 60;
        case UnitType::Support:  return 30;
        case UnitType::Assassin: return 30;
    }
    return 0;
}

// ═══════════════════════════════════════════════════════════════
// 创建单位
// ═══════════════════════════════════════════════════════════════

Unit* Synera::createUnitFromPool(UnitType type, bool isHero)
{
    if (isHero) {
        switch (type) {
            case UnitType::Warrior:  { auto u = std::make_unique<WarriorHero>();  Unit* p = u.get(); m_units.push_back(std::move(u)); return p; }
            case UnitType::Mage:     { auto u = std::make_unique<MageHero>();     Unit* p = u.get(); m_units.push_back(std::move(u)); return p; }
            case UnitType::Support:  { auto u = std::make_unique<SupportHero>();  Unit* p = u.get(); m_units.push_back(std::move(u)); return p; }
            case UnitType::Assassin: { auto u = std::make_unique<AssassinHero>(); Unit* p = u.get(); m_units.push_back(std::move(u)); return p; }
        }
    } else {
        switch (type) {
            case UnitType::Warrior:  { auto u = std::make_unique<WarriorEnemy>();  Unit* p = u.get(); m_units.push_back(std::move(u)); return p; }
            case UnitType::Mage:     { auto u = std::make_unique<MageEnemy>();     Unit* p = u.get(); m_units.push_back(std::move(u)); return p; }
            case UnitType::Support:  { auto u = std::make_unique<SupportEnemy>();  Unit* p = u.get(); m_units.push_back(std::move(u)); return p; }
            case UnitType::Assassin: { auto u = std::make_unique<AssassinEnemy>(); Unit* p = u.get(); m_units.push_back(std::move(u)); return p; }
        }
    }
    return nullptr;
}

// ═══════════════════════════════════════════════════════════════
// 开始战斗
// ═══════════════════════════════════════════════════════════════

void Synera::startBattle()
{
    // 每关敌方配置
    struct { UnitType type; int count; } enemyConfig[4] = {
        {UnitType::Warrior,  0},
        {UnitType::Mage,     0},
        {UnitType::Support,  0},
        {UnitType::Assassin, 0},
    };
    // 按关卡设定数量
    for (int i = 0; i < 4; ++i)
        enemyConfig[i].count = m_currentLevel; // 每关 N 个

    for (int i = 0; i < 4; ++i) {
        for (int c = 0; c < enemyConfig[i].count; ++c) {
            Unit* eu = createUnitFromPool(enemyConfig[i].type, false);
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
    }

    // 清空商店池中未使用的单位（标记消失）
    for (auto& slot : m_shop) slot.count = 0;

    m_showLevelLoss = false;
    m_phase = GamePhase::Battle;
    m_frameCounter = 0;
}

// ═══════════════════════════════════════════════════════════════
// 关卡结束
// ═══════════════════════════════════════════════════════════════

void Synera::endLevel(bool playerWon)
{
    if (playerWon) {
        // 回收场上存活英雄 — 扫描整个棋盘（英雄可能移动到敌方半场）
        for (int y = 0; y < Board::SIZE; ++y) {
            for (int x = 0; x < Board::SIZE; ++x) {
                Unit* u = m_board.getUnitAt(x, y);
                if (!u || dynamic_cast<Hero*>(u) == nullptr) continue;
                if (u->isDead() || u->isDisappeared()) continue;

                // 治疗 20，清空技能条
                u->heal(20);
                u->resetMana();
                u->resetMoveTimer();
                u->resetAttackTimer();

                // 放入对应类型的回收槽（优先第一行）
                int typeIdx = static_cast<int>(u->getType());
                bool placed = false;
                for (int row = 0; row < 2; ++row) {
                    int slotIdx = row * 4 + typeIdx;
                    if (m_recycleSlots[slotIdx] == nullptr) {
                        m_recycleSlots[slotIdx] = u;
                        placed = true;
                        break;
                    }
                }
                if (placed)
                    m_board.removeUnit(x, y);
                else
                    u->setDisappeared(true);
            }
        }
        m_gold += m_pendingGold;
        m_gold += 100; // 基础通关金币
    } else {
        // 失败：统计整个棋盘上剩余敌方（敌人可能移动到玩家半场）
        int remaining = 0;
        for (int y = 0; y < Board::SIZE; ++y) {
            for (int x = 0; x < Board::SIZE; ++x) {
                Unit* u = m_board.getUnitAt(x, y);
                if (u && !u->isDead() && !u->isDisappeared()
                    && dynamic_cast<Enemy*>(u) != nullptr)
                    ++remaining;
            }
        }
        m_playerHp -= remaining * 20;
        if (m_playerHp < 0) m_playerHp = 0;
        m_gold += m_pendingGold / 2;

        // 清除场上所有敌方角色
        for (int y = 0; y < Board::SIZE; ++y)
            for (int x = 0; x < Board::SIZE; ++x) {
                Unit* u = m_board.getUnitAt(x, y);
                if (u && dynamic_cast<Enemy*>(u) != nullptr)
                    u->setDisappeared(true);
            }

        // 场上英雄全部消失
        for (int y = 0; y < Board::SIZE; ++y)
            for (int x = 0; x < Board::SIZE; ++x) {
                Unit* u = m_board.getUnitAt(x, y);
                if (u && dynamic_cast<Hero*>(u) != nullptr)
                    u->setDisappeared(true);
            }

        // 若还有血量进入下一关，仍获得基础金币
        if (m_playerHp > 0)
            m_gold += 100;

        m_showLevelLoss = true;
    }

    m_pendingGold = 0;

    if (m_playerHp <= 0) {
        m_gameOver = true;
        m_playerVictory = false;
        return;
    }

    if (playerWon && m_currentLevel >= MAX_LEVEL) {
        m_gameOver = true;
        m_playerVictory = true;
        return;
    }

    ++m_currentLevel;
    initLevel();
}

// ═══════════════════════════════════════════════════════════════
// 游戏主循环 — 每帧运行
// ═══════════════════════════════════════════════════════════════

void Synera::gameLoop()
{
    float dt = m_frameClock.elapsed() / 1000.0f;
    m_frameClock.restart();
    Q_UNUSED(dt);

    if (m_phase == GamePhase::Battle && !m_gameOver) {
        ++m_frameCounter;
        processCombatFrame();
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
    renderRecycleSlots(painter);
    renderUnits(painter);
    renderShop(painter);
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
            if (m_draggedUnit && m_phase == GamePhase::Preparation && m_board.isPlayerHalf(y)) {
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
        case UnitType::Warrior:  return isHero ? QColor(210, 100, 30)  : QColor(180, 70, 20);
        case UnitType::Mage:     return isHero ? QColor(130, 80, 210)  : QColor(100, 55, 170);
        case UnitType::Support:  return isHero ? QColor(55, 170, 100)  : QColor(40, 140, 70);
        case UnitType::Assassin: return isHero ? QColor(200, 180, 40)  : QColor(160, 140, 20);
    }
    return QColor(128, 128, 128);
}

static QString typeLabel(UnitType t)
{
    switch (t) {
        case UnitType::Warrior:  return QString::fromUtf8("\346\210\230"); // 战
        case UnitType::Mage:     return QString::fromUtf8("\346\263\225"); // 法
        case UnitType::Support:  return QString::fromUtf8("\350\276\205"); // 辅
        case UnitType::Assassin: return QString::fromUtf8("\345\210\272"); // 刺
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

            // 角色标签
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
// 商店 (左侧)
// ═══════════════════════════════════════════════════════════════

QRect Synera::shopBuyRect(int index) const
{
    QRect base(SHOP_X, SHOP_Y + index * (SHOP_SLOT_H + 8), SHOP_WIDTH, SHOP_SLOT_H);
    return QRect(base.right() - 65, base.top() + 50, 60, 28);
}

int Synera::findShopSlotAt(const QPoint& pixel) const
{
    for (int i = 0; i < (int)m_shop.size(); ++i)
        if (QRect(SHOP_X, SHOP_Y + i * (SHOP_SLOT_H + 8), SHOP_WIDTH, SHOP_SLOT_H).contains(pixel))
            return i;
    return -1;
}

void Synera::renderShop(QPainter& painter)
{
    if (m_phase != GamePhase::Preparation) return;

    QFont titleFont;
    titleFont.setPixelSize(13);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(QColor(180, 180, 200));
    painter.drawText(SHOP_X, SHOP_Y - 12, "Shop");

    m_buyButtonRects.clear();

    for (int i = 0; i < (int)m_shop.size(); ++i) {
        QRect rc(SHOP_X, SHOP_Y + i * (SHOP_SLOT_H + 8), SHOP_WIDTH, SHOP_SLOT_H);
        const PoolSlot& slot = m_shop[i];

        // 底色
        painter.setBrush(QColor(40, 40, 50));
        painter.setPen(QPen(slot.count > 0 ? QColor(110, 110, 130) : QColor(60, 60, 70), 1));
        painter.drawRoundedRect(rc, 6, 6);

        // 角色色块
        QRect iconRect = rc.adjusted(8, 10, -8, -45);
        QColor fill = typeFillColor(slot.type, true);
        painter.setBrush(fill);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(iconRect, 6, 6);

        // 标签
        painter.setPen(Qt::white);
        QFont iconFont;
        iconFont.setPixelSize(24);
        iconFont.setBold(true);
        painter.setFont(iconFont);
        painter.drawText(iconRect, Qt::AlignCenter, typeLabel(slot.type));

        // 名称 + 库存
        const char* names[] = {"Warrior", "Mage", "Support", "Assassin"};
        QFont infoFont;
        infoFont.setPixelSize(11);
        infoFont.setBold(true);
        painter.setFont(infoFont);
        painter.setPen(QColor(200, 200, 220));
        QRect nameRect = rc.adjusted(8, rc.height() - 42, -8, -8);
        painter.drawText(nameRect, Qt::AlignHCenter | Qt::AlignTop,
                         QString("%1  x%2").arg(names[(int)slot.type]).arg(slot.count));

        // 价格 + 数值
        QFont statFont;
        statFont.setPixelSize(9);
        painter.setFont(statFont);
        painter.setPen(QColor(160, 160, 180));
        QString stats;
        switch (slot.type) {
            case UnitType::Warrior:  stats = "HP:100 ATK:20 Spd:30/60"; break;
            case UnitType::Mage:     stats = "HP:50  ATK:10 Spd:30/60"; break;
            case UnitType::Support:  stats = "HP:80  Heal:20 Spd:30/60"; break;
            case UnitType::Assassin: stats = "HP:40  ATK:50 Spd:30/40"; break;
        }
        painter.drawText(nameRect.adjusted(0, 14, 0, 0), Qt::AlignHCenter | Qt::AlignTop, stats);

        // 购买按钮
        int cost = heroCost(slot.type);
        QRect btnRect = shopBuyRect(i);
        m_buyButtonRects.push_back(btnRect);

        bool canBuy = (m_gold >= cost);
        painter.setBrush(canBuy ? QColor(55, 130, 55) : QColor(65, 65, 65));
        painter.setPen(QPen(canBuy ? QColor(90, 200, 90) : QColor(100, 100, 100), 1));
        painter.drawRoundedRect(btnRect, 4, 4);

        painter.setPen(Qt::white);
        QFont btnFont;
        btnFont.setPixelSize(11);
        btnFont.setBold(true);
        painter.setFont(btnFont);
        painter.drawText(btnRect, Qt::AlignCenter, QString("$%1").arg(cost));
    }
}

// ═══════════════════════════════════════════════════════════════
// 回收槽 (棋盘下方)
// ═══════════════════════════════════════════════════════════════

QRect Synera::recycleSlotRect(int row, int col) const
{
    int totalW = 4 * RECYCLE_SLOT_W + 3 * RECYCLE_SPACING;
    int startX = BOARD_OFFSET_X + (BOARD_PIXEL_SIZE - totalW) / 2;
    return QRect(startX + col * (RECYCLE_SLOT_W + RECYCLE_SPACING),
                 RECYCLE_Y + row * (RECYCLE_SLOT_H + 8),
                 RECYCLE_SLOT_W, RECYCLE_SLOT_H);
}

int Synera::findRecycleSlotAt(const QPoint& pixel) const
{
    for (int row = 0; row < 2; ++row)
        for (int col = 0; col < 4; ++col)
            if (recycleSlotRect(row, col).contains(pixel))
                return row * 4 + col;
    return -1;
}

void Synera::renderRecycleSlots(QPainter& painter)
{
    int totalW = 4 * RECYCLE_SLOT_W + 3 * RECYCLE_SPACING;
    int startX = BOARD_OFFSET_X + (BOARD_PIXEL_SIZE - totalW) / 2;

    // 标题常驻
    QFont titleFont;
    titleFont.setPixelSize(11);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(QColor(160, 160, 180));
    painter.drawText(startX, RECYCLE_Y - 8, "Recycle");

    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 4; ++col) {
            QRect rc = recycleSlotRect(row, col);
            int idx = row * 4 + col;
            Unit* u = m_recycleSlots[idx];

            if (u) {
                // 有单位：实心色块 + 蓝色边框
                UnitType t = u->getType();
                QColor fill = typeFillColor(t, true);
                painter.setBrush(fill);
                painter.setPen(QPen(QColor(100, 170, 255), 1));
                painter.drawRoundedRect(rc, 4, 4);

                painter.setPen(Qt::white);
                QFont f;
                f.setPixelSize(13);
                f.setBold(true);
                painter.setFont(f);
                painter.drawText(rc, Qt::AlignCenter, typeLabel(t));

                // HP 小条
                int barH = 4;
                int barY = rc.bottom() - barH - 1;
                double ratio = (double)u->getHp() / u->getMaxHp();
                QRect barBg(rc.left() + 2, barY, rc.width() - 4, barH);
                painter.setBrush(QColor(30, 30, 30));
                painter.setPen(Qt::NoPen);
                painter.drawRoundedRect(barBg, 1, 1);
                QColor hpC = ratio > 0.5 ? QColor(80, 210, 80) : ratio > 0.25 ? QColor(210, 190, 30) : QColor(210, 55, 55);
                painter.setBrush(hpC);
                painter.drawRoundedRect(QRect(barBg.left(), barY, (int)(barBg.width() * ratio), barH), 1, 1);
            } else {
                // 空槽：透明填充 + 白色边框
                painter.setBrush(Qt::NoBrush);
                painter.setPen(QPen(QColor(255, 255, 255, 60), 1));
                painter.drawRoundedRect(rc, 4, 4);
            }
        }
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

    // ── 顶部信息栏：HP(左) 关卡(中) 金币(右) ──
    int infoY = 20;
    QFont infoFont;
    infoFont.setPixelSize(17);
    infoFont.setBold(true);
    painter.setFont(infoFont);

    // HP - 棋盘左上方
    QColor hpColor = m_playerHp > 30 ? QColor(80, 220, 80) : QColor(220, 60, 60);
    painter.setPen(hpColor);
    painter.drawText(BOARD_OFFSET_X, infoY, QString("HP: %1").arg(m_playerHp));

    // 关卡 - 棋盘上方中间
    painter.setPen(QColor(255, 210, 80));
    QFont lvlFont;
    lvlFont.setPixelSize(18);
    lvlFont.setBold(true);
    painter.setFont(lvlFont);
    QString lvlText = QString("Level %1 / %2").arg(m_currentLevel).arg(MAX_LEVEL);
    painter.drawText(BOARD_OFFSET_X + BOARD_PIXEL_SIZE / 2 - 50, infoY, lvlText);

    // 金币 - 棋盘右上方
    painter.setPen(QColor(255, 210, 50));
    painter.setFont(infoFont);
    QString goldText = QString("Gold: %1").arg(m_gold);
    painter.drawText(BOARD_OFFSET_X + BOARD_PIXEL_SIZE - 100, infoY, goldText);

    // ── 阶段标题 ──
    QFont uiFont;
    uiFont.setPixelSize(16);
    uiFont.setBold(true);
    painter.setFont(uiFont);

    QString status;
    if (m_gameOver) {
        if (m_playerVictory) {
            status = "VICTORY! All levels cleared!";
            painter.setPen(QColor(80, 255, 120));
        } else {
            status = "DEFEAT - GAME OVER";
            painter.setPen(QColor(255, 80, 80));
        }
    } else if (m_phase == GamePhase::Preparation) {
        if (m_showLevelLoss) {
            status = "PREPARATION PHASE  —  Level Failed!";
            painter.setPen(QColor(255, 100, 80));
        } else {
            status = "PREPARATION PHASE";
            painter.setPen(QColor(255, 200, 100));
        }
    } else {
        status = "BATTLE IN PROGRESS";
        painter.setPen(QColor(255, 120, 80));
    }
    painter.drawText(textX, BOARD_OFFSET_Y + 30, status);

    // ── 开始战斗按钮 ──
    if (m_phase == GamePhase::Preparation && !m_gameOver) {
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
            painter.drawText(textX, btnY + btnH + 18, "Buy & drag units to the board");
        }
    }

    // ── 图例 ──
    int legendY = m_phase == GamePhase::Preparation
        ? m_startButtonRect.bottom() + 55 : BOARD_OFFSET_Y + 75;
    QFont legFont;
    legFont.setPixelSize(11);
    painter.setFont(legFont);
    painter.setPen(QColor(170, 170, 190));
    painter.drawText(textX, legendY, "Legend:");

    struct { QString label; QColor color; } legend[] = {
        {QString::fromUtf8("\342\227\217 Hero Warrior"),  typeFillColor(UnitType::Warrior, true)},
        {QString::fromUtf8("\342\227\217 Hero Mage"),     typeFillColor(UnitType::Mage, true)},
        {QString::fromUtf8("\342\227\217 Hero Support"),  typeFillColor(UnitType::Support, true)},
        {QString::fromUtf8("\342\227\217 Hero Assassin"), typeFillColor(UnitType::Assassin, true)},
        {QString::fromUtf8("\342\227\217 Enemy Warrior"),  typeFillColor(UnitType::Warrior, false)},
        {QString::fromUtf8("\342\227\217 Enemy Mage"),     typeFillColor(UnitType::Mage, false)},
        {QString::fromUtf8("\342\227\217 Enemy Support"),  typeFillColor(UnitType::Support, false)},
        {QString::fromUtf8("\342\227\217 Enemy Assassin"), typeFillColor(UnitType::Assassin, false)},
    };
    for (int i = 0; i < 8; ++i) {
        painter.setPen(legend[i].color);
        painter.drawText(textX + 8, legendY + 20 + i * 18, legend[i].label);
    }

    // ── 存活单位列表 ──
    int unitListY = legendY + 20 + 8 * 18 + 20;
    painter.setPen(QColor(190, 190, 210));
    QFont listTitleFont;
    listTitleFont.setPixelSize(13);
    listTitleFont.setBold(true);
    painter.setFont(listTitleFont);
    painter.drawText(textX, unitListY, "Alive Units:");

    unitListY += 22;
    QFont listFont;
    listFont.setPixelSize(10);
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
        unitListY += 16;
    }

    // ── 关卡失败覆盖文字 ──
    if (m_showLevelLoss && m_phase == GamePhase::Preparation) {
        QFont failFont;
        failFont.setPixelSize(36);
        failFont.setBold(true);
        painter.setFont(failFont);
        painter.setPen(QColor(255, 60, 60));
        int cx = BOARD_OFFSET_X + BOARD_PIXEL_SIZE / 2;
        int cy = BOARD_OFFSET_Y + BOARD_PIXEL_SIZE / 2;
        QRect overlayRect(cx - 300, cy - 40, 600, 80);
        painter.drawText(overlayRect, Qt::AlignCenter, "Level Failed!");
    }

    // ── 帮助 ──
    int helpY = unitListY + 16;
    painter.setPen(QColor(140, 140, 160));
    QFont helpFont;
    helpFont.setPixelSize(11);
    painter.setFont(helpFont);
    if (m_phase == GamePhase::Preparation) {
        painter.drawText(textX, helpY, "Buy heroes from shop (left)");
        painter.drawText(textX, helpY + 16, "Drag from shop / recycle to board");
        painter.drawText(textX, helpY + 32, "Press R to full reset");
    } else {
        painter.drawText(textX, helpY, "Auto-combat in progress...");
        painter.drawText(textX, helpY + 16, "Press R to full reset");
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

    if (m_phase == GamePhase::Preparation) {
        // 开始战斗按钮
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

        // 商店购买按钮
        for (int i = 0; i < (int)m_buyButtonRects.size(); ++i) {
            if (m_buyButtonRects[i].contains(pos)) {
                int cost = heroCost(m_shop[i].type);
                if (m_gold >= cost) {
                    m_gold -= cost;
                    m_shop[i].count++;
                }
                return;
            }
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
    // 1) 商店池
    int shopIdx = findShopSlotAt(mousePos);
    if (shopIdx >= 0 && m_shop[shopIdx].count > 0) {
        Unit* u = createUnitFromPool(m_shop[shopIdx].type, true);
        m_draggedUnit = u;
        m_dragFromShopIndex = shopIdx;
        m_dragFromRecycleIndex = -1;
        m_dragCurrentPos = mousePos;
        m_shop[shopIdx].count--;
        return;
    }

    // 2) 回收槽
    int recycleIdx = findRecycleSlotAt(mousePos);
    if (recycleIdx >= 0 && m_recycleSlots[recycleIdx] != nullptr) {
        m_draggedUnit = m_recycleSlots[recycleIdx];
        m_recycleSlots[recycleIdx] = nullptr;
        m_dragFromShopIndex = -1;
        m_dragFromRecycleIndex = recycleIdx;
        m_dragCurrentPos = mousePos;
        return;
    }

    // 3) 棋盘上的英雄
    Unit* clicked = findUnitAtPixel(mousePos);
    if (clicked && dynamic_cast<Hero*>(clicked) && !clicked->isDisappeared()) {
        m_draggedUnit = clicked;
        m_dragFromShopIndex = -1;
        m_dragFromRecycleIndex = -1;
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

    // ── 放回商店池 ──
    int shopIdx = findShopSlotAt(mousePos);
    if (shopIdx >= 0 && m_shop[shopIdx].type == m_draggedUnit->getType()) {
        m_shop[shopIdx].count++;
        m_draggedUnit->setDisappeared(true);
        m_draggedUnit = nullptr;
        m_dragFromShopIndex = -1;
        m_dragFromRecycleIndex = -1;
        update();
        return;
    }

    // ── 放回回收槽 ──
    int recycleIdx = findRecycleSlotAt(mousePos);
    if (recycleIdx >= 0 && m_recycleSlots[recycleIdx] == nullptr) {
        int typeIdx = static_cast<int>(m_draggedUnit->getType());
        int slotTypeIdx = recycleIdx % 4;
        if (typeIdx == slotTypeIdx) {
            m_recycleSlots[recycleIdx] = m_draggedUnit;
            m_draggedUnit = nullptr;
            m_dragFromShopIndex = -1;
            m_dragFromRecycleIndex = -1;
            update();
            return;
        }
    }

    // ── 放到棋盘 ──
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
        m_board.placeUnit(m_draggedUnit, bestGx, bestGy);
        m_dragFromShopIndex = -1;
        m_dragFromRecycleIndex = -1;
    } else {
        // 没有合法位置 → 返还来源
        if (m_dragFromShopIndex >= 0) {
            m_shop[m_dragFromShopIndex].count++;
            m_draggedUnit->setDisappeared(true);
        } else if (m_dragFromRecycleIndex >= 0) {
            m_recycleSlots[m_dragFromRecycleIndex] = m_draggedUnit;
        } else {
            // 从棋盘来，放回原位
            m_board.placeUnit(m_draggedUnit, m_draggedUnit->getPosition().x,
                              m_draggedUnit->getPosition().y);
        }
    }

    m_draggedUnit = nullptr;
    m_dragFromShopIndex = -1;
    m_dragFromRecycleIndex = -1;
    update();
}

// ═══════════════════════════════════════════════════════════════
// 自动战斗 — 每帧调用
// ═══════════════════════════════════════════════════════════════

void Synera::processBurningTick(std::vector<Unit*>& alive)
{
    for (Unit* u : alive) {
        if (u->isBurning()) {
            u->tickBurning();
            if (u->isDead()) {
                if (dynamic_cast<Enemy*>(u) != nullptr)
                    m_pendingGold += enemyGoldValue(u->getType());
                m_board.removeUnit(u->getPosition().x, u->getPosition().y);
            }
        }
    }
}

void Synera::processCombatFrame()
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

    // 燃烧伤害（每 BURNING_INTERVAL 帧）
    if (m_frameCounter % BURNING_INTERVAL == 0)
        processBurningTick(alive);

    for (Unit* u : alive) {
        if (u->isDead() || u->isDisappeared()) continue;
        Position pos = u->getPosition();

        u->incrementTimers();

        // ── 法力值满 → 释放技能 ──
        if (u->getMana() >= Unit::MAX_MANA) {
            // 记录技能前存活敌方
            std::vector<Unit*> enemiesBeforeSkill;
            for (Unit* eu : alive) {
                if (!eu->isDead() && !eu->isDisappeared()
                    && dynamic_cast<Enemy*>(eu) != nullptr)
                    enemiesBeforeSkill.push_back(eu);
            }

            u->useSkill(m_board, alive);
            u->resetMana();
            u->resetMoveTimer();
            u->resetAttackTimer();

            // 技能击杀的敌方计入金币
            for (Unit* eu : enemiesBeforeSkill) {
                if (eu->isDead())
                    m_pendingGold += enemyGoldValue(eu->getType());
            }
            continue;
        }

        if (u->canHeal()) {
            // ── 辅助逻辑 ──
            Unit* healTarget = findHealTarget(u);
            bool inRange = healTarget && manhattanDist(pos, healTarget->getPosition()) <= u->getAttackRange();

            // 治疗（独立计时器）
            if (inRange && u->getAttackTimer() >= u->getAttackSpeed()) {
                healTarget->heal(u->getHealAmount());
                u->gainMana();
                u->resetAttackTimer();
            } else if (!inRange) {
                u->resetAttackTimer();
            }

            // 移动（独立计时器）
            if (u->getMoveTimer() >= u->getMoveSpeed()) {
                Unit* target = healTarget;
                if (!target) target = findNearestEnemyFor(u);
                if (target) {
                    Position next = moveStepToward(pos, target->getPosition());
                    if (!(next == pos)) moves.push_back({u, next});
                }
                u->resetMoveTimer();
            }
        } else {
            // ── 战士 / 法师 / 刺客 ──
            Unit* target = findNearestEnemyFor(u);
            if (target) {
                bool inRange = canAttack(u, target);

                // 攻击（独立计时器）
                if (inRange && u->getAttackTimer() >= u->getAttackSpeed()) {
                    u->attack(*target);
                    u->gainMana();

                    if (target->isDead()) {
                        bool isEnemy = dynamic_cast<Enemy*>(target) != nullptr;
                        if (isEnemy) m_pendingGold += enemyGoldValue(target->getType());
                        m_board.removeUnit(target->getPosition().x, target->getPosition().y);
                    }
                    u->resetAttackTimer();
                } else if (!inRange) {
                    u->resetAttackTimer();
                }

                // 移动（独立计时器）
                if (u->getMoveTimer() >= u->getMoveSpeed()) {
                    Position next = moveStepToward(pos, target->getPosition());
                    if (!(next == pos)) moves.push_back({u, next});
                    u->resetMoveTimer();
                }
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

    checkLevelEnd();
}

// ═══════════════════════════════════════════════════════════════
// 索敌
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
            if (uIsHero == isHero) continue;

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
            if (u->getHp() >= u->getMaxHp()) continue;
            bool uIsHero = dynamic_cast<Hero*>(u) != nullptr;
            if (uIsHero != isHero) continue;

            int d = manhattanDist(support->getPosition(), u->getPosition());
            int xd = std::abs(support->getPosition().x - u->getPosition().x);

            bool better = false;
            if (d < bestDist) better = true;
            else if (d == bestDist) {
                int tr = (u->getType() == UnitType::Warrior) ? 0 :
                         (u->getType() == UnitType::Mage) ? 1 :
                         (u->getType() == UnitType::Assassin) ? 2 : 3;
                int br = (bestType == UnitType::Warrior) ? 0 :
                         (bestType == UnitType::Mage) ? 1 :
                         (bestType == UnitType::Assassin) ? 2 : 3;
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
// 关卡结束判定
// ═══════════════════════════════════════════════════════════════

void Synera::checkLevelEnd()
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
    bool heroLost = (!hasHeroAny || !hasHeroNonSupport);
    bool enemyLost = (!hasEnemyAny || !hasEnemyNonSupport);

    if (heroLost || enemyLost) {
        // 如果敌方只剩辅助，视为已击败这些辅助（给金币）
        if (enemyLost) {
            for (auto& u : m_units) {
                if (u->isDisappeared() || u->isDead()) continue;
                if (dynamic_cast<Enemy*>(u.get()) == nullptr) continue;
                m_pendingGold += enemyGoldValue(u->getType());
            }
        }
        endLevel(enemyLost); // enemyLost = player won
    }
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
