#include "synera.h"
#include "ui_synera.h"
#include "hero.h"
#include "enemy.h"
#include "weapon.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFont>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <set>

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
    , m_gold(1360)
    , m_pendingGold(0)
    , m_recycleSlots(16, nullptr)
    , m_draggedUnit(nullptr)
    , m_dragFromShopIndex(-1)
    , m_dragFromRecycleIndex(-1)
    , m_populationCap(5)
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
    m_gold = 1360;
    m_pendingGold = 0;
    m_populationCap = 5;

    // 初始化英雄信息面板：4 种类型
    m_shop.clear();
    m_shop.push_back({UnitType::Warrior, 0});
    m_shop.push_back({UnitType::Mage, 0});
    m_shop.push_back({UnitType::Support, 0});
    m_shop.push_back({UnitType::Assassin, 0});

    m_recruitRects.clear();
    m_recruitSlots.clear();
    for (int i = 0; i < 5; ++i)
        m_recruitSlots.push_back({UnitType::Warrior, 0, true});

    refreshRecruitment();
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
    m_hitEffects.clear();
    m_slashEffects.clear();
    m_pendingDamageEvents.clear();

    refreshRecruitment();

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

void Synera::refreshRecruitment()
{
    UnitType types[] = {UnitType::Warrior, UnitType::Mage, UnitType::Support, UnitType::Assassin};
    for (auto& slot : m_recruitSlots) {
        slot.type = types[std::rand() % 4];
        int base = heroCost(slot.type);
        int fluctuation = 5 * (std::rand() % 5) * ((std::rand() % 3) - 1);
        slot.price = base + fluctuation;
        if (slot.price < 1) slot.price = 1;
        slot.empty = false;
    }
}

int Synera::enemyGoldValue(const Unit* u) const
{
    int base = 0;
    switch (u->getType()) {
        case UnitType::Warrior:  base = 80; break;
        case UnitType::Mage:     base = 60; break;
        case UnitType::Support:  base = 30; break;
        case UnitType::Assassin: base = 30; break;
        case UnitType::Boss:     base = 200; break;
    }
    return base * (u->getStarLevel() / 2 + 1);
}

// ═══════════════════════════════════════════════════════════════
// 创建单位
// ═══════════════════════════════════════════════════════════════

Unit* Synera::createUnitFromPool(UnitType type, bool isHero, int starLevel, bool isBoss)
{
    if (isHero) {
        switch (type) {
            case UnitType::Warrior:  { auto u = std::make_unique<WarriorHero>(starLevel);  Unit* p = u.get(); m_units.push_back(std::move(u)); return p; }
            case UnitType::Mage:     { auto u = std::make_unique<MageHero>(starLevel);     Unit* p = u.get(); m_units.push_back(std::move(u)); return p; }
            case UnitType::Support:  { auto u = std::make_unique<SupportHero>(starLevel);  Unit* p = u.get(); m_units.push_back(std::move(u)); return p; }
            case UnitType::Assassin: { auto u = std::make_unique<AssassinHero>(starLevel); Unit* p = u.get(); m_units.push_back(std::move(u)); return p; }
            default: break;
        }
    } else {
        if (isBoss) {
            auto u = std::make_unique<BossEnemy>();
            Unit* p = u.get();
            m_units.push_back(std::move(u));
            return p;
        }
        switch (type) {
            case UnitType::Warrior:  { auto u = std::make_unique<WarriorEnemy>(starLevel);  Unit* p = u.get(); m_units.push_back(std::move(u)); return p; }
            case UnitType::Mage:     { auto u = std::make_unique<MageEnemy>(starLevel);     Unit* p = u.get(); m_units.push_back(std::move(u)); return p; }
            case UnitType::Support:  { auto u = std::make_unique<SupportEnemy>(starLevel);  Unit* p = u.get(); m_units.push_back(std::move(u)); return p; }
            case UnitType::Assassin: { auto u = std::make_unique<AssassinEnemy>(starLevel); Unit* p = u.get(); m_units.push_back(std::move(u)); return p; }
            default: break;
        }
    }
    return nullptr;
}

Unit* Synera::createUpgradedHero(UnitType type, int starLevel)
{
    return createUnitFromPool(type, true, starLevel);
}

bool Synera::tryStarUp(int boardX, int boardY, Unit* draggedUnit)
{
    Unit* targetUnit = m_board.getUnitAt(boardX, boardY);
    if (!targetUnit || !draggedUnit) return false;
    if (targetUnit == draggedUnit) return false;

    // Both must be heroes
    Hero* h1 = dynamic_cast<Hero*>(draggedUnit);
    Hero* h2 = dynamic_cast<Hero*>(targetUnit);
    if (!h1 || !h2) return false;

    // Same name and same full-star level (starLevel/2)
    if (draggedUnit->getName() != targetUnit->getName()) return false;
    int starLv1 = draggedUnit->getStarLevel();
    int starLv2 = targetUnit->getStarLevel();
    if (starLv1 / 2 != starLv2 / 2) return false;

    // Max half-star level is 6 (3 full stars)
    if (starLv1 >= 6 || starLv2 >= 6) return false;

    int newStarLevel = std::max(starLv1, starLv2) + 1; // +half star
    UnitType type = draggedUnit->getType();

    // Remove target from board; dragged unit is already detached from its source
    m_board.removeUnit(boardX, boardY);

    // Delete both old units
    draggedUnit->setDisappeared(true);
    targetUnit->setDisappeared(true);

    // Create new upgraded unit
    Unit* newUnit = createUpgradedHero(type, newStarLevel);

    // Place on board
    m_board.placeUnit(newUnit, boardX, boardY);

    // Mana: Assassin gets full, others start at 0
    if (type == UnitType::Assassin) {
        for (int i = newUnit->getMana(); i < Unit::MAX_MANA; ++i)
            newUnit->gainMana();
    } else {
        newUnit->resetMana();
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════
// 开始战斗
// ═══════════════════════════════════════════════════════════════

void Synera::startBattle()
{
    auto placeRandom = [&](Unit* eu) {
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
    };

    if (m_currentLevel <= 3) {
        // 关卡 1-3：每种类型 N 个 0 星敌方
        UnitType types[] = {UnitType::Warrior, UnitType::Mage, UnitType::Support, UnitType::Assassin};
        for (int i = 0; i < 4; ++i) {
            for (int c = 0; c < m_currentLevel; ++c) {
                Unit* eu = createUnitFromPool(types[i], false, 0);
                placeRandom(eu);
            }
        }
    } else if (m_currentLevel == 4) {
        // 关卡 4：每种类型各 2 个 2 星敌方
        UnitType types[] = {UnitType::Warrior, UnitType::Mage, UnitType::Support, UnitType::Assassin};
        for (int i = 0; i < 4; ++i) {
            for (int c = 0; c < 2; ++c) {
                Unit* eu = createUnitFromPool(types[i], false, 4);
                placeRandom(eu);
            }
        }
    } else if (m_currentLevel == 5) {
        // 关卡 5：每种类型各 1 个 3 星敌方 + 1 个 Boss
        UnitType types[] = {UnitType::Warrior, UnitType::Mage, UnitType::Support, UnitType::Assassin};
        for (int i = 0; i < 4; ++i) {
            Unit* eu = createUnitFromPool(types[i], false, 6);
            placeRandom(eu);
        }
        Unit* boss = createUnitFromPool(UnitType::Boss, false, 6, true);
        placeRandom(boss);
    }

    m_showLevelLoss = false;
    m_hitEffects.clear();
    m_slashEffects.clear();
    m_pendingDamageEvents.clear();
    m_phase = GamePhase::Battle;
    m_frameCounter = 0;
}

// ═══════════════════════════════════════════════════════════════
// 关卡结束
// ═══════════════════════════════════════════════════════════════

void Synera::endLevel(bool playerWon)
{
    // 清空战斗中的伤害/治疗显示残留
    m_hitEffects.clear();
    m_slashEffects.clear();
    m_pendingDamageEvents.clear();

    if (playerWon) {
        // 收集场上存活英雄并按星数、类型排序
        std::vector<Unit*> survivingHeroes;
        for (int y = 0; y < Board::SIZE; ++y) {
            for (int x = 0; x < Board::SIZE; ++x) {
                Unit* u = m_board.getUnitAt(x, y);
                if (!u || dynamic_cast<Hero*>(u) == nullptr) continue;
                if (u->isDead() || u->isDisappeared()) continue;
                survivingHeroes.push_back(u);
            }
        }

        // 排序：星数高优先，同星按类型：战士>法师>辅助>刺客
        std::sort(survivingHeroes.begin(), survivingHeroes.end(),
            [](Unit* a, Unit* b) {
                if (a->getStarLevel() != b->getStarLevel())
                    return a->getStarLevel() > b->getStarLevel();
                return static_cast<int>(a->getType()) < static_cast<int>(b->getType());
            });

        for (Unit* u : survivingHeroes) {
            u->heal(20);
            u->resetMoveTimer();
            u->resetAttackTimer();

            bool placed = false;
            for (int i = 0; i < 16; ++i) {
                if (m_recycleSlots[i] == nullptr) {
                    m_recycleSlots[i] = u;
                    placed = true;
                    break;
                }
            }
            if (placed)
                m_board.removeUnit(u->getPosition().x, u->getPosition().y);
            else
                u->setDisappeared(true);
        }
        // 金币利息（若有下一关，未使用金币变1.5倍）
        if (m_currentLevel < MAX_LEVEL)
            m_gold = static_cast<int>(m_gold * 1.5);

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

    if (playerWon) {
        if (m_currentLevel >= MAX_LEVEL) {
            m_gameOver = true;
            m_playerVictory = true;
            return;
        }
        ++m_currentLevel;
        if (m_currentLevel == 5)
            m_gold += 800;
    }

    initLevel();
}

// ═══════════════════════════════════════════════════════════════
// 存档 / 读档
// ═══════════════════════════════════════════════════════════════

static const char* SAVE_PATH = "savegame.json";

void Synera::saveGame(const QString& filePath)
{
    QJsonObject root;

    root["gold"] = m_gold;
    root["currentLevel"] = m_currentLevel;
    root["playerHp"] = m_playerHp;
    root["pendingGold"] = m_pendingGold;
    root["populationCap"] = m_populationCap;

    // 招募区
    QJsonArray shopArr;
    for (int i = 0; i < (int)m_recruitSlots.size(); ++i) {
        QJsonObject s;
        s["type"] = static_cast<int>(m_recruitSlots[i].type);
        s["price"] = m_recruitSlots[i].price;
        s["empty"] = m_recruitSlots[i].empty;
        shopArr.append(s);
    }
    root["shop"] = shopArr;

    // 棋盘上的英雄
    QJsonArray boardArr;
    for (int y = 0; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            Unit* u = m_board.getUnitAt(x, y);
            if (!u || u->isDead() || u->isDisappeared()) continue;
            Hero* h = dynamic_cast<Hero*>(u);
            if (!h) continue;

            QJsonObject bu;
            bu["x"] = x;
            bu["y"] = y;
            bu["type"] = static_cast<int>(u->getType());
            bu["star"] = u->getStarLevel();
            bu["hp"] = u->getHp();
            bu["maxHp"] = u->getMaxHp();
            bu["mana"] = u->getMana();
            bu["burning"] = u->getBurningTurns();
            if (u->getEquipment())
                bu["equip"] = QString("%1 %2").arg(QString::fromStdString(u->getEquipment()->getName()))
                                               .arg(u->getEquipment()->getDamage());
            boardArr.append(bu);
        }
    }
    root["boardUnits"] = boardArr;

    // 回收槽中的英雄
    QJsonArray recycleArr;
    for (int i = 0; i < 16; ++i) {
        Unit* u = m_recycleSlots[i];
        if (!u || u->isDead() || u->isDisappeared()) {
            recycleArr.append(QJsonValue::Null);
            continue;
        }
        QJsonObject ru;
        ru["slot"] = i;
        ru["type"] = static_cast<int>(u->getType());
        ru["star"] = u->getStarLevel();
        ru["hp"] = u->getHp();
        ru["maxHp"] = u->getMaxHp();
        ru["mana"] = u->getMana();
        ru["burning"] = u->getBurningTurns();
        if (u->getEquipment())
            ru["equip"] = QString("%1 %2").arg(QString::fromStdString(u->getEquipment()->getName()))
                                           .arg(u->getEquipment()->getDamage());
        recycleArr.append(ru);
    }
    root["recycleUnits"] = recycleArr;

    QJsonDocument doc(root);
    QFile file(filePath);
    if (file.exists())
        file.remove();
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

void Synera::loadGame(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (doc.isNull()) return;

    QJsonObject root = doc.object();

    // 清空当前状态
    initGame();

    // 恢复全局状态
    m_gold = root["gold"].toInt(360);
    m_currentLevel = root["currentLevel"].toInt(1);
    m_playerHp = root["playerHp"].toInt(100);
    m_pendingGold = root["pendingGold"].toInt(0);
    m_populationCap = root["populationCap"].toInt(5);

    // 恢复招募区
    QJsonArray shopArr = root["shop"].toArray();
    for (int i = 0; i < shopArr.size() && i < (int)m_recruitSlots.size(); ++i) {
        QJsonObject so = shopArr[i].toObject();
        m_recruitSlots[i].type = static_cast<UnitType>(so["type"].toInt(0));
        m_recruitSlots[i].price = so["price"].toInt(heroCost(m_recruitSlots[i].type));
        m_recruitSlots[i].empty = so["empty"].toBool(false);
    }

    // 辅助：根据 type/star/hp/mana/equip 创建并初始化英雄
    auto createHeroFromData = [&](const QJsonObject& o) -> Unit* {
        auto t = static_cast<UnitType>(o["type"].toInt());
        int star = o["star"].toInt(0);
        Unit* u = createUnitFromPool(t, true, star);
        u->setHp(o["hp"].toInt(u->getMaxHp()));
        u->setMaxHp(o["maxHp"].toInt(u->getMaxHp()));
        u->resetMana();
        int mana = o["mana"].toInt(0);
        for (int i = 0; i < mana; ++i) u->gainMana();
        int burning = o["burning"].toInt(0);
        if (burning > 0) u->applyBurning(burning);
        QString equipStr = o["equip"].toString();
        if (!equipStr.isEmpty()) {
            auto parts = equipStr.split(' ');
            if (parts.size() >= 2) {
                auto w = std::make_unique<Weapon>(parts[0].toStdString(), parts[1].toInt());
                Weapon* wp = w.get();
                m_weapons.push_back(std::move(w));
                u->setEquipment(wp);
            }
        }
        return u;
    };

    // 恢复棋盘单位
    QJsonArray boardArr = root["boardUnits"].toArray();
    for (auto val : boardArr) {
        QJsonObject o = val.toObject();
        Unit* u = createHeroFromData(o);
        m_board.placeUnit(u, o["x"].toInt(), o["y"].toInt());
    }

    // 恢复回收槽单位
    QJsonArray recycleArr = root["recycleUnits"].toArray();
    for (int i = 0; i < recycleArr.size() && i < 16; ++i) {
        if (recycleArr[i].isNull()) continue;
        QJsonObject o = recycleArr[i].toObject();
        Unit* u = createHeroFromData(o);
        int slot = o["slot"].toInt(i);
        if (slot >= 0 && slot < 16)
            m_recycleSlots[slot] = u;
    }
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
    renderHeroInfo(painter);
    renderRecruitment(painter);
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

static void drawStar(QPainter& painter, const QPointF& center, double radius, int fillMode)
{
    // fillMode: 0=empty, 1=half, 2=full
    const double pi = 3.14159265358979323846;
    QPainterPath path;
    int n = 5;

    double outerR = radius;
    double innerR = radius * 0.4;
    for (int i = 0; i < n * 2; ++i) {
        double angle = -pi / 2 + i * pi / n;
        double r = (i % 2 == 0) ? outerR : innerR;
        double px = center.x() + r * std::cos(angle);
        double py = center.y() + r * std::sin(angle);
        if (i == 0) path.moveTo(px, py);
        else path.lineTo(px, py);
    }
    path.closeSubpath();

    if (fillMode == 2) {
        painter.setBrush(QColor(255, 210, 50));
        painter.setPen(QPen(QColor(200, 160, 30), 1));
        painter.drawPath(path);
    } else if (fillMode == 1) {
        // clip to left half
        painter.save();
        QPainterPath clipPath;
        clipPath.addRect(QRectF(center.x() - radius, center.y() - radius,
                                radius, radius * 2));
        painter.setClipPath(clipPath, Qt::IntersectClip);
        painter.setBrush(QColor(255, 210, 50));
        painter.setPen(QPen(QColor(200, 160, 30), 1));
        painter.drawPath(path);
        painter.restore();
    } else {
        painter.setBrush(QColor(50, 50, 60));
        painter.setPen(QPen(QColor(80, 80, 90), 1));
        painter.drawPath(path);
    }
}

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

            // 星数图标（仅英雄显示，在格子左侧）
            if (isHero) {
                double starR = 5.0;
                double starSpacing = 14.0;
                double startY = rc.center().y() - starSpacing;
                double starX = rc.left() + starR + 4;
                int halfStars = unit->getStarLevel();
                for (int s = 0; s < 3; ++s) {
                    QPointF starCenter(starX, startY + s * starSpacing);
                    int mode = 0;
                    if (s < halfStars / 2) mode = 2;
                    else if (s == halfStars / 2 && halfStars % 2 == 1) mode = 1;
                    drawStar(painter, starCenter, starR, mode);
                }
            }

            // 伤害/治疗浮动文本（右上角）
            int hitOffsetY = 0;
            for (auto& he : m_hitEffects) {
                if (he.cellX != x || he.cellY != y) continue;
                if (!he.alive) continue;
                QColor hitColor = he.amount >= 0 ? QColor(80, 255, 80) : QColor(255, 80, 80);
                QString hitText = he.amount >= 0
                    ? QString("+%1").arg(he.amount)
                    : QString("-%1").arg(-he.amount);
                painter.setPen(hitColor);
                QFont hitFont;
                hitFont.setPixelSize(11);
                hitFont.setBold(true);
                painter.setFont(hitFont);
                QRect hitRect(rc.right() - 32, rc.top() - 2 + hitOffsetY, 36, 14);
                painter.drawText(hitRect, Qt::AlignRight | Qt::AlignTop, hitText);
                hitOffsetY += 14;
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════
// 英雄信息面板 (左侧，只读)
// ═══════════════════════════════════════════════════════════════

QRect Synera::recruitSlotRect(int index) const
{
    return QRect(LEFT_PANEL_X, RECRUIT_START_Y + index * (RECRUIT_SLOT_H + RECRUIT_SPACING),
                 LEFT_PANEL_W, RECRUIT_SLOT_H);
}

int Synera::findRecruitSlotAt(const QPoint& pixel) const
{
    for (int i = 0; i < (int)m_recruitSlots.size(); ++i)
        if (recruitSlotRect(i).contains(pixel))
            return i;
    return -1;
}

void Synera::renderHeroInfo(QPainter& painter)
{
    if (m_phase != GamePhase::Preparation) return;

    // 标题
    QFont titleFont;
    titleFont.setPixelSize(12);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(QColor(180, 180, 200));
    painter.drawText(LEFT_PANEL_X, BOARD_OFFSET_Y - 12, "Hero Info");

    for (int i = 0; i < (int)m_shop.size(); ++i) {
        const PoolSlot& slot = m_shop[i];
        int rowY = INFO_PANEL_Y + i * (INFO_PANEL_H + INFO_SPACING);

        QRect panelRect(LEFT_PANEL_X, rowY, LEFT_PANEL_W, INFO_PANEL_H);
        painter.setBrush(QColor(34, 34, 44));
        painter.setPen(QPen(QColor(60, 60, 75), 1));
        painter.drawRoundedRect(panelRect, 4, 4);

        // 类型色块 + 标签
        QRect colorRect(panelRect.left() + 4, panelRect.top() + 4, 22, 22);
        QColor fill = typeFillColor(slot.type, true);
        painter.setBrush(fill);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(colorRect, 3, 3);

        painter.setPen(Qt::white);
        QFont iconFont;
        iconFont.setPixelSize(12);
        iconFont.setBold(true);
        painter.setFont(iconFont);
        painter.drawText(colorRect, Qt::AlignCenter, typeLabel(slot.type));

        // 名称
        const char* names[] = {"Warrior", "Mage", "Support", "Assassin"};
        QFont nameFont;
        nameFont.setPixelSize(9);
        nameFont.setBold(true);
        painter.setFont(nameFont);
        painter.setPen(QColor(200, 200, 220));
        painter.drawText(colorRect.right() + 6, panelRect.top() + 14, names[(int)slot.type]);

        // 属性数据（8px 字体）
        QFont statFont;
        statFont.setPixelSize(8);
        painter.setFont(statFont);
        painter.setPen(QColor(170, 170, 190));
        int sx = panelRect.left() + 6;
        int sy = panelRect.top() + 28;
        int lh = 11;
        int baseCost = heroCost(slot.type);
        switch (slot.type) {
            case UnitType::Warrior:
                painter.drawText(sx, sy,        "HP:100  ATK:20");
                painter.drawText(sx, sy + lh,   "Rng:1  Spd:60/120");
                break;
            case UnitType::Mage:
                painter.drawText(sx, sy,        "HP:50  ATK:10");
                painter.drawText(sx, sy + lh,   "Rng:4  Spd:60/120");
                break;
            case UnitType::Support:
                painter.drawText(sx, sy,        "HP:80  Heal:20");
                painter.drawText(sx, sy + lh,   "Rng:2  Spd:60/120");
                break;
            case UnitType::Assassin:
                painter.drawText(sx, sy,        "HP:15  ATK:50");
                painter.drawText(sx, sy + lh,   "Rng:1  Spd:60/80");
                break;
        }
        painter.setPen(QColor(255, 210, 50));
        painter.drawText(sx, sy + lh * 2, QString("Base: $%1").arg(baseCost));
    }
}

// ═══════════════════════════════════════════════════════════════
// 招募区 (左侧信息面板下方)
// ═══════════════════════════════════════════════════════════════

void Synera::renderRecruitment(QPainter& painter)
{
    if (m_phase != GamePhase::Preparation) return;

    int titleY = RECRUIT_START_Y - 22;

    // 标题
    QFont titleFont;
    titleFont.setPixelSize(12);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(QColor(180, 180, 200));
    painter.drawText(LEFT_PANEL_X, titleY, "Recruit");

    // 刷新按钮（标题右侧）
    int refreshW = 62, refreshH = 18;
    QRect refreshRect(LEFT_PANEL_X + LEFT_PANEL_W - refreshW, titleY - 14, refreshW, refreshH);
    m_refreshButtonRect = refreshRect;

    bool canRefresh = (m_gold >= 15);
    painter.setBrush(canRefresh ? QColor(55, 110, 55) : QColor(55, 55, 55));
    painter.setPen(QPen(canRefresh ? QColor(80, 180, 80) : QColor(90, 90, 90), 1));
    painter.drawRoundedRect(refreshRect, 3, 3);

    painter.setPen(canRefresh ? Qt::white : QColor(150, 150, 150));
    QFont rfFont;
    rfFont.setPixelSize(9);
    rfFont.setBold(true);
    painter.setFont(rfFont);
    painter.drawText(refreshRect, Qt::AlignCenter, "Refresh $15");

    // 招募槽
    m_recruitRects.clear();
    for (int i = 0; i < (int)m_recruitSlots.size(); ++i) {
        const RecruitSlot& slot = m_recruitSlots[i];
        QRect rc = recruitSlotRect(i);
        m_recruitRects.push_back(rc);

        if (slot.empty) {
            painter.setBrush(QColor(30, 30, 38));
            painter.setPen(QPen(QColor(55, 55, 65), 1));
            painter.drawRoundedRect(rc, 4, 4);
            painter.setPen(QColor(100, 100, 110));
            QFont emptyFont;
            emptyFont.setPixelSize(10);
            painter.setFont(emptyFont);
            painter.drawText(rc, Qt::AlignCenter, "- empty -");
        } else {
            painter.setBrush(QColor(38, 38, 48));
            painter.setPen(QPen(QColor(70, 70, 85), 1));
            painter.drawRoundedRect(rc, 4, 4);

            // 角色色块
            QRect colorRect(rc.left() + 4, rc.top() + 4, 28, rc.height() - 8);
            QColor fill = typeFillColor(slot.type, true);
            painter.setBrush(fill);
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(colorRect, 4, 4);

            // 类型标签
            painter.setPen(Qt::white);
            QFont iconFont;
            iconFont.setPixelSize(14);
            iconFont.setBold(true);
            painter.setFont(iconFont);
            painter.drawText(colorRect, Qt::AlignCenter, typeLabel(slot.type));

            // 名称
            const char* names[] = {"Warrior", "Mage", "Support", "Assassin"};
            QFont nameFont;
            nameFont.setPixelSize(9);
            nameFont.setBold(true);
            painter.setFont(nameFont);
            painter.setPen(QColor(200, 200, 220));
            painter.drawText(colorRect.right() + 6, rc.top() + 14, names[(int)slot.type]);

            // 价格（右侧）
            bool canBuy = (m_gold >= slot.price);
            painter.setPen(canBuy ? QColor(80, 220, 80) : QColor(200, 80, 80));
            QFont priceFont;
            priceFont.setPixelSize(10);
            priceFont.setBold(true);
            painter.setFont(priceFont);
            QRect priceRect(rc.right() - 55, rc.top(), 50, rc.height());
            painter.drawText(priceRect, Qt::AlignRight | Qt::AlignVCenter, QString("$%1").arg(slot.price));
        }
    }

    // 人口上限升级按钮（招募区下方）
    int popBtnY = RECRUIT_START_Y + 5 * (RECRUIT_SLOT_H + RECRUIT_SPACING) + 8;
    int popCost = 100 * (m_populationCap - 4);
    QRect popBtnRect(LEFT_PANEL_X, popBtnY, 80, 26);
    m_popUpgradeButtonRect = popBtnRect;

    bool canBuyPop = (m_gold >= popCost);
    painter.setBrush(canBuyPop ? QColor(45, 80, 130) : QColor(55, 55, 55));
    painter.setPen(QPen(canBuyPop ? QColor(80, 150, 220) : QColor(90, 90, 90), 1));
    painter.drawRoundedRect(popBtnRect, 4, 4);

    painter.setPen(canBuyPop ? Qt::white : QColor(150, 150, 150));
    QFont popBtnFont;
    popBtnFont.setPixelSize(10);
    popBtnFont.setBold(true);
    painter.setFont(popBtnFont);
    painter.drawText(popBtnRect, Qt::AlignCenter, QString("Pop+ $%1").arg(popCost));
}

// ═══════════════════════════════════════════════════════════════
// 回收槽 (棋盘下方)
// ═══════════════════════════════════════════════════════════════

QRect Synera::recycleSlotRect(int row, int col) const
{
    int totalW = 8 * RECYCLE_SLOT_W + 7 * RECYCLE_SPACING;
    int startX = BOARD_OFFSET_X + (BOARD_PIXEL_SIZE - totalW) / 2;
    return QRect(startX + col * (RECYCLE_SLOT_W + RECYCLE_SPACING),
                 RECYCLE_Y + row * (RECYCLE_SLOT_H + 8),
                 RECYCLE_SLOT_W, RECYCLE_SLOT_H);
}

int Synera::findRecycleSlotAt(const QPoint& pixel) const
{
    for (int row = 0; row < 2; ++row)
        for (int col = 0; col < 8; ++col)
            if (recycleSlotRect(row, col).contains(pixel))
                return row * 8 + col;
    return -1;
}

void Synera::renderRecycleSlots(QPainter& painter)
{
    int totalW = 8 * RECYCLE_SLOT_W + 7 * RECYCLE_SPACING;
    int startX = BOARD_OFFSET_X + (BOARD_PIXEL_SIZE - totalW) / 2;

    // 标题常驻
    QFont titleFont;
    titleFont.setPixelSize(11);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(QColor(160, 160, 180));
    painter.drawText(startX, RECYCLE_Y - 8, "Recycle");

    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 8; ++col) {
            QRect rc = recycleSlotRect(row, col);
            int idx = row * 8 + col;
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

                // 星数图标
                double starR = 3.5;
                double starSpacing = 8.0;
                double startY = rc.center().y() - starSpacing;
                double starX = rc.left() + starR + 3;
                int halfStars = u->getStarLevel();
                for (int s = 0; s < 3; ++s) {
                    QPointF starCenter(starX, startY + s * starSpacing);
                    int mode = 0;
                    if (s < halfStars / 2) mode = 2;
                    else if (s == halfStars / 2 && halfStars % 2 == 1) mode = 1;
                    drawStar(painter, starCenter, starR, mode);
                }
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

    // 人口 - 金币右侧
    int boardHeroCount = countBoardHeroes();
    painter.setPen(QColor(160, 200, 255));
    QFont popFont;
    popFont.setPixelSize(14);
    popFont.setBold(true);
    painter.setFont(popFont);
    QString popText = QString("Pop: %1/%2").arg(boardHeroCount).arg(m_populationCap);
    painter.drawText(BOARD_OFFSET_X + BOARD_PIXEL_SIZE + 35, infoY, popText);

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
        painter.drawText(textX, helpY, "Click recruit slots to buy (left)");
        painter.drawText(textX, helpY + 16, "Purchased heroes appear in recycle area");
        painter.drawText(textX, helpY + 32, "Drag from recycle to board / info to sell");
        painter.drawText(textX, helpY + 48, "F5 Save  F9 Load  R Reset");
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

int Synera::countBoardHeroes() const
{
    int count = 0;
    for (int y = 0; y < Board::SIZE; ++y)
        for (int x = 0; x < Board::SIZE; ++x) {
            Unit* u = m_board.getUnitAt(x, y);
            if (u && !u->isDead() && !u->isDisappeared()
                && dynamic_cast<Hero*>(u) != nullptr)
                ++count;
        }
    return count;
}

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

        // 招募区刷新按钮
        if (m_refreshButtonRect.contains(pos)) {
            if (m_gold >= 15) {
                m_gold -= 15;
                refreshRecruitment();
            }
            return;
        }

        // 招募槽点击购买
        int recruitIdx = findRecruitSlotAt(pos);
        if (recruitIdx >= 0 && !m_recruitSlots[recruitIdx].empty) {
            int cost = m_recruitSlots[recruitIdx].price;
            if (m_gold >= cost) {
                // 找一个空回收槽
                int emptySlot = -1;
                for (int i = 0; i < 16; ++i) {
                    if (m_recycleSlots[i] == nullptr) {
                        emptySlot = i;
                        break;
                    }
                }
                if (emptySlot >= 0) {
                    m_gold -= cost;
                    Unit* u = createUnitFromPool(m_recruitSlots[recruitIdx].type, true);
                    m_recycleSlots[emptySlot] = u;
                    m_recruitSlots[recruitIdx].empty = true;
                }
            }
            return;
        }

        // 人口上限升级按钮
        if (m_popUpgradeButtonRect.contains(pos)) {
            int popCost = 100 * (m_populationCap - 4);
            if (m_gold >= popCost) {
                m_gold -= popCost;
                m_populationCap++;
            }
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
    // 1) 回收槽
    int recycleIdx = findRecycleSlotAt(mousePos);
    if (recycleIdx >= 0 && m_recycleSlots[recycleIdx] != nullptr) {
        m_draggedUnit = m_recycleSlots[recycleIdx];
        m_recycleSlots[recycleIdx] = nullptr;
        m_dragFromShopIndex = -1;
        m_dragFromRecycleIndex = recycleIdx;
        m_dragCurrentPos = mousePos;
        return;
    }

    // 2) 棋盘上的英雄
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

    // ── 卖回英雄信息面板 ──
    for (int i = 0; i < (int)m_shop.size(); ++i) {
        QRect infoRect(LEFT_PANEL_X, INFO_PANEL_Y + i * (INFO_PANEL_H + INFO_SPACING),
                       LEFT_PANEL_W, INFO_PANEL_H);
        if (infoRect.contains(mousePos) && m_shop[i].type == m_draggedUnit->getType()) {
            m_gold += heroCost(m_shop[i].type); // 返还基础价格
            m_draggedUnit->setDisappeared(true);
            m_draggedUnit = nullptr;
            m_dragFromShopIndex = -1;
            m_dragFromRecycleIndex = -1;
            update();
            return;
        }
    }

    // ── 放回回收槽 (含升星) ──
    int recycleIdx = findRecycleSlotAt(mousePos);
    if (recycleIdx >= 0) {
        Unit* slotUnit = m_recycleSlots[recycleIdx];
        if (slotUnit) {
            // Check for star-up in recycle slot
            Hero* h1 = dynamic_cast<Hero*>(m_draggedUnit);
            Hero* h2 = dynamic_cast<Hero*>(slotUnit);
            if (h1 && h2
                && m_draggedUnit->getName() == slotUnit->getName()
                && m_draggedUnit->getStarLevel() / 2 == slotUnit->getStarLevel() / 2
                && m_draggedUnit->getStarLevel() < 6
                && slotUnit->getStarLevel() < 6) {
                int newStarLevel = std::max(m_draggedUnit->getStarLevel(), slotUnit->getStarLevel()) + 1;
                UnitType type = m_draggedUnit->getType();
                m_draggedUnit->setDisappeared(true);
                slotUnit->setDisappeared(true);
                m_recycleSlots[recycleIdx] = nullptr;
                Unit* newUnit = createUpgradedHero(type, newStarLevel);
                m_recycleSlots[recycleIdx] = newUnit;
                if (type == UnitType::Assassin) {
                    for (int i = newUnit->getMana(); i < Unit::MAX_MANA; ++i)
                        newUnit->gainMana();
                }
                m_draggedUnit = nullptr;
                m_dragFromShopIndex = -1;
                m_dragFromRecycleIndex = -1;
                update();
                return;
            }
        } else {
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
    int starUpGx = -1, starUpGy = -1, bestStarUpOverlap = 0;
    for (int y = Board::SIZE / 2; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            QRect cr = cellRect(x, y);
            QRect inter = unitRect.intersected(cr);
            if (inter.isEmpty()) continue;
            int overlap = inter.width() * inter.height();
            if (overlap <= unitArea / 2) continue;

            if (m_board.isOccupied(x, y)) {
                Unit* occupant = m_board.getUnitAt(x, y);
                if (occupant && dynamic_cast<Hero*>(occupant)
                    && dynamic_cast<Hero*>(m_draggedUnit)
                    && occupant->getName() == m_draggedUnit->getName()
                    && occupant->getStarLevel() / 2 == m_draggedUnit->getStarLevel() / 2
                    && occupant->getStarLevel() < 6
                    && m_draggedUnit->getStarLevel() < 6
                    && overlap > bestStarUpOverlap) {
                    bestStarUpOverlap = overlap;
                    starUpGx = x; starUpGy = y;
                }
            } else {
                if (overlap > bestOverlap) {
                    bestOverlap = overlap;
                    bestGx = x; bestGy = y;
                }
            }
        }
    }

    if (starUpGx >= 0) {
        if (tryStarUp(starUpGx, starUpGy, m_draggedUnit)) {
            m_dragFromShopIndex = -1;
            m_dragFromRecycleIndex = -1;
        } else {
            // Star-up failed — return dragged unit to source
            if (m_dragFromRecycleIndex >= 0) {
                m_recycleSlots[m_dragFromRecycleIndex] = m_draggedUnit;
            } else {
                m_board.placeUnit(m_draggedUnit, m_draggedUnit->getPosition().x,
                                  m_draggedUnit->getPosition().y);
            }
            m_dragFromShopIndex = -1;
            m_dragFromRecycleIndex = -1;
        }
    } else if (bestGx >= 0) {
        // 检查人口上限（仅当拖拽来源不是棋盘时）
        if (m_dragFromRecycleIndex >= 0) {
            if (countBoardHeroes() >= m_populationCap) {
                // 人口已满，返还回收槽
                m_recycleSlots[m_dragFromRecycleIndex] = m_draggedUnit;
                m_draggedUnit = nullptr;
                m_dragFromShopIndex = -1;
                m_dragFromRecycleIndex = -1;
                update();
                return;
            }
        }
        m_board.placeUnit(m_draggedUnit, bestGx, bestGy);
        m_dragFromShopIndex = -1;
        m_dragFromRecycleIndex = -1;
    } else {
        // 没有合法位置 → 返还来源
        if (m_dragFromRecycleIndex >= 0) {
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
                    m_pendingGold += enemyGoldValue(u);
                m_board.removeUnit(u->getPosition().x, u->getPosition().y);
            }
        }
    }
}

void Synera::processAssassinSkills(std::vector<Unit*>& alive)
{
    // 收集有技能且范围内有敌方的刺客
    std::vector<Unit*> skillAssassins;
    for (Unit* u : alive) {
        if (u->isDead() || u->isDisappeared()) continue;
        if (u->getType() != UnitType::Assassin) continue;
        if (u->getMana() < u->getMaxMana()) continue;
        bool isHero = dynamic_cast<Hero*>(u) != nullptr;
        bool hasTarget = false;
        for (Unit* eu : alive) {
            if (eu == u || eu->isDead() || eu->isDisappeared()) continue;
            bool euIsHero = dynamic_cast<Hero*>(eu) != nullptr;
            if (euIsHero == isHero) continue;
            if (manhattanDist(u->getPosition(), eu->getPosition()) <= 2) {
                hasTarget = true; break;
            }
        }
        if (hasTarget) skillAssassins.push_back(u);
    }
    if (skillAssassins.empty()) return;

    std::set<Unit*> deadSet;

    for (Unit* assassin : skillAssassins) {
        if (deadSet.count(assassin)) continue;
        if (assassin->isDead() || assassin->isDisappeared()) continue;

        bool isHero = dynamic_cast<Hero*>(assassin) != nullptr;
        int dmg = static_cast<int>(80 * (1 + (assassin->getStarLevel() / 2) * 0.5));
        Position ap = assassin->getPosition();

        // 索敌：绝对距离最近，横向距离破平
        Unit* target = nullptr;
        int bestDistSq = 999999, bestXDist = 999;
        for (Unit* eu : alive) {
            if (eu == assassin || eu->isDead() || eu->isDisappeared()) continue;
            if (deadSet.count(eu)) continue;
            bool euIsHero = dynamic_cast<Hero*>(eu) != nullptr;
            if (euIsHero == isHero) continue;
            if (manhattanDist(ap, eu->getPosition()) > 2) continue;
            int dx = ap.x - eu->getPosition().x;
            int dy = ap.y - eu->getPosition().y;
            int distSq = dx * dx + dy * dy;
            int xDist = std::abs(dx);
            if (distSq < bestDistSq || (distSq == bestDistSq && xDist < bestXDist)) {
                bestDistSq = distSq; bestXDist = xDist; target = eu;
            }
        }
        if (!target) continue; // 无有效目标，保留技能点

        // 互杀检测：目标也是有技能的刺客
        bool mutualKill = false;
        if (target->getType() == UnitType::Assassin && target->getMana() >= target->getMaxMana()) {
            for (Unit* sa : skillAssassins) {
                if (sa == target && !deadSet.count(target)) {
                    mutualKill = true; break;
                }
            }
        }

        if (mutualKill) {
            deadSet.insert(assassin);
            deadSet.insert(target);
            bool aEnemy = dynamic_cast<Enemy*>(assassin) != nullptr;
            bool tEnemy = dynamic_cast<Enemy*>(target) != nullptr;
            if (aEnemy) m_pendingGold += enemyGoldValue(assassin);
            if (tEnemy) m_pendingGold += enemyGoldValue(target);
            m_pendingDamageEvents[assassin].push_back(-assassin->getHp());
            m_pendingDamageEvents[target].push_back(-target->getHp());
            assassin->takeDamage(assassin->getHp());
            target->takeDamage(target->getHp());
            assassin->resetMana();
            target->resetMana();
            m_board.removeUnit(ap.x, ap.y);
            m_board.removeUnit(target->getPosition().x, target->getPosition().y);
            continue;
        }

        // 对目标造成伤害
        int beforeHp = target->getHp();
        target->takeDamage(dmg);
        int dealt = beforeHp - target->getHp();
        m_pendingDamageEvents[target].push_back(-dealt);
        assassin->resetMana();

        if (target->isDead()) {
            deadSet.insert(target);
            bool tEnemy = dynamic_cast<Enemy*>(target) != nullptr;
            if (tEnemy) m_pendingGold += enemyGoldValue(target);
            m_board.removeUnit(target->getPosition().x, target->getPosition().y);
        }
    }
}

void Synera::processCombatFrame()
{
    // 清理过期特效
    m_hitEffects.erase(
        std::remove_if(m_hitEffects.begin(), m_hitEffects.end(),
            [this](const HitEffect& e) { return m_frameCounter >= e.startFrame + e.duration; }),
        m_hitEffects.end());
    m_slashEffects.erase(
        std::remove_if(m_slashEffects.begin(), m_slashEffects.end(),
            [this](const SlashEffect& e) { return m_frameCounter >= e.startFrame + e.duration; }),
        m_slashEffects.end());

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
    bool burningFrame = (m_frameCounter % BURNING_INTERVAL == 0);
    if (burningFrame)
        processBurningTick(alive);

    // 燃烧结算后立刻处理刺客技能
    if (burningFrame)
        processAssassinSkills(alive);

    for (Unit* u : alive) {
        if (u->isDead() || u->isDisappeared()) continue;
        Position pos = u->getPosition();

        u->incrementTimers();

        // ── 法力值满 → 释放技能（刺客由 processAssassinSkills 统一处理）──
        if (u->getMana() >= u->getMaxMana() && u->getType() != UnitType::Assassin) {
            bool hasValidTarget = false;
            bool isSupport = u->canHeal();

            if (isSupport) {
                bool isHero = dynamic_cast<Hero*>(u) != nullptr;
                for (Unit* au : alive) {
                    if (au == u || au->isDead() || au->isDisappeared()) continue;
                    bool auIsHero = dynamic_cast<Hero*>(au) != nullptr;
                    if (auIsHero != isHero) continue;
                    if (au->getHp() < au->getMaxHp()) { hasValidTarget = true; break; }
                }
            } else if (u->getType() == UnitType::Mage) {
                // 法师技能需周围5×5有敌方
                bool isHero = dynamic_cast<Hero*>(u) != nullptr;
                int range = 2;
                for (Unit* eu : alive) {
                    if (eu == u || eu->isDead() || eu->isDisappeared()) continue;
                    bool euIsHero = dynamic_cast<Hero*>(eu) != nullptr;
                    if (euIsHero == isHero) continue;
                    int dx = std::abs(u->getPosition().x - eu->getPosition().x);
                    int dy = std::abs(u->getPosition().y - eu->getPosition().y);
                    if (dx <= range && dy <= range) { hasValidTarget = true; break; }
                }
            } else {
                bool isHero = dynamic_cast<Hero*>(u) != nullptr;
                for (Unit* eu : alive) {
                    if (eu == u || eu->isDead() || eu->isDisappeared()) continue;
                    bool euIsHero = dynamic_cast<Hero*>(eu) != nullptr;
                    if (euIsHero != isHero) { hasValidTarget = true; break; }
                }
            }

            if (!hasValidTarget) {
                // 无有效目标，保留技能点，继续移动/攻击
            } else {
                // 记录技能前 HP
                std::map<Unit*, int> hpBefore;
                for (Unit* au : alive) {
                    if (!au->isDead() && !au->isDisappeared())
                        hpBefore[au] = au->getHp();
                }

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

                for (auto& kv : hpBefore) {
                    if (kv.first->isDead() || kv.first->isDisappeared()) continue;
                    int diff = kv.first->getHp() - kv.second;
                    if (diff != 0)
                        m_pendingDamageEvents[kv.first].push_back(diff);
                }

                for (Unit* eu : enemiesBeforeSkill) {
                    if (eu->isDead())
                        m_pendingGold += enemyGoldValue(eu);
                }
                continue;
            }
        }

        if (u->canHeal()) {
            // ── 辅助逻辑 ──
            Unit* healTarget = findHealTarget(u);
            bool inRange = healTarget && manhattanDist(pos, healTarget->getPosition()) <= u->getAttackRange();

            // 治疗（独立计时器）
            if (inRange && u->getAttackTimer() >= u->getAttackSpeed()) {
                int healed = healTarget->heal(u->getHealAmount());
                m_pendingDamageEvents[healTarget].push_back(healed);
                u->gainMana();
                u->resetAttackTimer();
            } else if (!inRange) {
                u->resetAttackTimer();
            }

            // 移动：优先接近受伤友方，否则靠近最近友方
            if (u->getMoveTimer() >= u->getMoveSpeed()) {
                if (healTarget) {
                    Position next = moveStepToward(pos, healTarget->getPosition());
                    if (!(next == pos)) moves.push_back({u, next});
                } else {
                    Unit* ally = findNearestAlly(u);
                    if (ally) {
                        Position next = moveStepTowardAlly(pos, ally->getPosition());
                        if (!(next == pos)) moves.push_back({u, next});
                    }
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
                    int dealt = u->attack(*target);
                    u->gainMana();
                    m_pendingDamageEvents[target].push_back(-dealt);

                    if (target->isDead()) {
                        bool isEnemy = dynamic_cast<Enemy*>(target) != nullptr;
                        if (isEnemy) m_pendingGold += enemyGoldValue(target);
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

    // 刷新收集到的全部单位（含移动后新位置）
    alive.clear();
    for (int y = 0; y < Board::SIZE; ++y)
        for (int x = 0; x < Board::SIZE; ++x) {
            Unit* u = m_board.getUnitAt(x, y);
            if (u && !u->isDead() && !u->isDisappeared())
                alive.push_back(u);
        }
    flushDamageEvents(alive);

    checkLevelEnd();
}

// ═══════════════════════════════════════════════════════════════
// 伤害显示辅助
// ═══════════════════════════════════════════════════════════════

int Synera::fastestAttackSpeed() const
{
    int fastest = 999;
    for (int y = 0; y < Board::SIZE; ++y)
        for (int x = 0; x < Board::SIZE; ++x) {
            Unit* u = m_board.getUnitAt(x, y);
            if (u && !u->isDead() && !u->isDisappeared()) {
                if (u->getAttackSpeed() < fastest)
                    fastest = u->getAttackSpeed();
            }
        }
    return (fastest == 999) ? 60 : fastest;
}

void Synera::flushDamageEvents(const std::vector<Unit*>& alive)
{
    if (m_pendingDamageEvents.empty()) return;
    int fastAtk = fastestAttackSpeed();

    for (auto& kv : m_pendingDamageEvents) {
        Unit* u = kv.first;
        if (u->isDead() || u->isDisappeared()) continue;
        int count = (int)kv.second.size();
        if (count == 0) continue;
        int duration = fastAtk / count - 3;
        if (duration < 0) duration = 0;

        for (int amt : kv.second) {
            m_hitEffects.push_back({
                u->getPosition().x,
                u->getPosition().y,
                amt,
                m_frameCounter,
                duration,
                true
            });
        }
    }
    m_pendingDamageEvents.clear();
}

// ═══════════════════════════════════════════════════════════════
// 索敌
// ═══════════════════════════════════════════════════════════════

Unit* Synera::findNearestEnemyFor(Unit* unit) const
{
    bool isHero = dynamic_cast<Hero*>(unit) != nullptr;
    Unit* best = nullptr;
    int bestDistSq = 999999;
    int bestXDist = 999;

    for (int y = 0; y < Board::SIZE; ++y)
        for (int x = 0; x < Board::SIZE; ++x) {
            Unit* u = m_board.getUnitAt(x, y);
            if (!u || u == unit || u->isDead() || u->isDisappeared()) continue;
            bool uIsHero = dynamic_cast<Hero*>(u) != nullptr;
            if (uIsHero == isHero) continue;

            int dx = unit->getPosition().x - u->getPosition().x;
            int dy = unit->getPosition().y - u->getPosition().y;
            int distSq = dx * dx + dy * dy;
            int xDist = std::abs(dx);

            if (distSq < bestDistSq || (distSq == bestDistSq && xDist < bestXDist)) {
                bestDistSq = distSq;
                bestXDist = xDist;
                best = u;
            }
        }
    return best;
}

Unit* Synera::findHealTarget(Unit* support) const
{
    bool isHero = dynamic_cast<Hero*>(support) != nullptr;
    Unit* best = nullptr;
    int bestDistSq = 999999;
    UnitType bestType = UnitType::Support;
    int bestXDist = 999;

    for (int y = 0; y < Board::SIZE; ++y)
        for (int x = 0; x < Board::SIZE; ++x) {
            Unit* u = m_board.getUnitAt(x, y);
            if (!u || u == support || u->isDead() || u->isDisappeared()) continue;
            if (u->getHp() >= u->getMaxHp()) continue;
            bool uIsHero = dynamic_cast<Hero*>(u) != nullptr;
            if (uIsHero != isHero) continue;

            int dx = support->getPosition().x - u->getPosition().x;
            int dy = support->getPosition().y - u->getPosition().y;
            int distSq = dx * dx + dy * dy;
            int xd = std::abs(dx);

            bool better = false;
            if (distSq < bestDistSq) better = true;
            else if (distSq == bestDistSq) {
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
                best = u; bestDistSq = distSq; bestType = u->getType(); bestXDist = xd;
            }
        }
    return best;
}

Unit* Synera::findNearestAlly(Unit* unit) const
{
    bool isHero = dynamic_cast<Hero*>(unit) != nullptr;
    Unit* best = nullptr;
    int bestDistSq = 999999;
    int bestXDist = 999;

    for (int y = 0; y < Board::SIZE; ++y)
        for (int x = 0; x < Board::SIZE; ++x) {
            Unit* u = m_board.getUnitAt(x, y);
            if (!u || u == unit || u->isDead() || u->isDisappeared()) continue;
            bool uIsHero = dynamic_cast<Hero*>(u) != nullptr;
            if (uIsHero != isHero) continue;

            int dx = unit->getPosition().x - u->getPosition().x;
            int dy = unit->getPosition().y - u->getPosition().y;
            int distSq = dx * dx + dy * dy;
            int xDist = std::abs(dx);

            if (distSq < bestDistSq || (distSq == bestDistSq && xDist < bestXDist)) {
                bestDistSq = distSq;
                bestXDist = xDist;
                best = u;
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

Position Synera::moveStepTowardAlly(const Position& from, const Position& to) const
{
    // 优先向右，其次向前/后，最后向左
    if (to.x > from.x) {
        Position next(from.x + 1, from.y);
        if (m_board.isValidPosition(next.x, next.y) && !m_board.isOccupied(next.x, next.y))
            return next;
    }
    if (to.y != from.y) {
        int dy = (to.y > from.y) ? 1 : -1;
        Position next(from.x, from.y + dy);
        if (m_board.isValidPosition(next.x, next.y) && !m_board.isOccupied(next.x, next.y))
            return next;
    }
    if (to.x < from.x) {
        Position next(from.x - 1, from.y);
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
    bool hasHeroAny = false, hasEnemyAny = false;

    for (auto& u : m_units) {
        if (u->isDisappeared() || u->isDead()) continue;
        bool isH = dynamic_cast<Hero*>(u.get()) != nullptr;
        if (isH) {
            hasHeroAny = true;
        } else {
            hasEnemyAny = true;
        }
    }

    // 只有一方全部阵亡才结束战斗
    if (!hasHeroAny || !hasEnemyAny) {
        endLevel(!hasEnemyAny); // enemy all dead = player won
    }
}

// ═══════════════════════════════════════════════════════════════
// 键盘
// ═══════════════════════════════════════════════════════════════

void Synera::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_R) {
        initGame();
    } else if (event->key() == Qt::Key_F5 && m_phase == GamePhase::Preparation && !m_gameOver) {
        saveGame(SAVE_PATH);
    } else if (event->key() == Qt::Key_F9 && m_phase == GamePhase::Preparation) {
        loadGame(SAVE_PATH);
    }
    QMainWindow::keyPressEvent(event);
}
