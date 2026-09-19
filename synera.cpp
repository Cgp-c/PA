#include "synera.h"
#include "ui_synera.h"
#include "hero.h"
#include "enemy.h"
#include "unitvisuals.h"
#include "weapon.h"
#include "equipsynthwindow.h"
#include "custombattlewindow.h"
#include "equipicons.h"
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFont>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QCoreApplication>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <set>
//存档以后在战斗失败以后判定？？？存疑

// 前向声明：合成配方查询
static std::string getSynthesisResultName(const std::string& a, const std::string& b);

// ═══════════════════════════════════════════════════════════════
// 构造 / 析构
// ═══════════════════════════════════════════════════════════════

Synera::Synera(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Synera - Auto Chess Arena");
    resize(880, 720);
    setMouseTracking(true);

    initGame();

    m_gameTimer = new QTimer(this);
    connect(m_gameTimer, &QTimer::timeout, this, &Synera::gameLoop);
    m_gameTimer->start(16);
    m_frameClock.start();

    // 自动化验证钩子：SYNERA_SHOW_CUSTOM=1 时启动即打开自定义难度窗口
    if (qEnvironmentVariableIsSet("SYNERA_SHOW_CUSTOM"))
        showCustomBattleWindow();
}

Synera::~Synera()
{
    delete m_equipSynthWindow;    // 主窗口析构时释放合成树窗口
    delete m_customBattleWindow;  // 释放自定义难度窗口
    delete ui;
}

void Synera::showEquipSynthWindow()
{
    if (!m_equipSynthWindow) {
        // 延迟创建：首次点击按钮时才实例化
        m_equipSynthWindow = new EquipSynthWindow(nullptr); // 不设 parent，完全独立
    }
    m_equipSynthWindow->show();
    m_equipSynthWindow->raise();
    m_equipSynthWindow->activateWindow();
}

void Synera::showCustomBattleWindow()
{
    if (!m_customBattleWindow) {
        // 延迟创建：首次点击按钮时才实例化（与装备合成树同模式）
        m_customBattleWindow = new CustomBattleWindow(nullptr);
        connect(m_customBattleWindow, &CustomBattleWindow::startRequested,
                this, &Synera::startCustomBattle);
    }
    m_customBattleWindow->show();
    m_customBattleWindow->raise();
    m_customBattleWindow->activateWindow();
}

// ═══════════════════════════════════════════════════════════════
// 自定义战斗：按窗口配置生成敌方编成，独立于关卡系统
// ═══════════════════════════════════════════════════════════════

void Synera::startCustomBattle()
{
    if (!m_customBattleWindow) return;
    if (m_phase != GamePhase::Preparation || m_gameOver) return;
    if (countBoardHeroes() <= 0) return;          // 至少一名上场英雄
    if (!m_customBattleWindow->isValid()) return; // 窗口侧校验

    // 防御性再校验：字段范围 + 总量上限（不信任跨窗口数据）
    const auto specs = m_customBattleWindow->specs();
    int total = 0;
    for (const auto& sp : specs) {
        if (sp.type < 0 || sp.type > static_cast<int>(UnitType::Boss)) return;
        if (sp.star < 0 || sp.star > 3) return;
        if (sp.count < 1) return;
        total += sp.count;
        if (total > CustomBattleWindow::MAX_ENEMIES) return;
    }
    if (total < 1) return;

    for (const auto& sp : specs) {
        bool isBoss = static_cast<UnitType>(sp.type) == UnitType::Boss;
        for (int i = 0; i < sp.count; ++i) {
            // 整星制：starLevel = 整星 × 2（与关卡 4=2星/6=3星一致）
            Unit* eu = createUnitFromPool(static_cast<UnitType>(sp.type), false,
                                          isBoss ? 0 : sp.star * 2, isBoss);
            if (!placeEnemyRandom(eu)) break;   // 理论上到不了这里（已校验上限）
        }
    }

    m_customBattle = true;
    m_showLevelLoss = false;
    for (int i = 0; i < 5; ++i) m_bondActive[i] = false;
    m_phase = GamePhase::Battle;
    m_frameCounter = 0;
    m_burnTickCount = 0;
}

// ═══════════════════════════════════════════════════════════════
// 初始化
// ═══════════════════════════════════════════════════════════════

void Synera::initGame()
{
    m_units.clear();
    m_weapons.clear();
    m_board.clear();
    m_recycleSlots.assign(16, nullptr); // 重置并保证尺寸为 16
    m_draggedUnit = nullptr;
    m_dragFromRecycleIndex = -1;
    m_phase = GamePhase::Preparation;
    m_gameOver = false;
    m_playerVictory = false;
    m_showLevelLoss = false;
    m_frameCounter = 0;
    m_burnTickCount = 0;
    m_customBattle = false;
    m_currentLevel = 1;
    m_playerHp = 100;
    m_gold = 8000;
    m_pendingGold = 0;
    m_populationCap = 4;
    m_hitEffects.clear();
    m_slashEffects.clear();
    m_projectileEffects.clear();
    m_healEffects.clear();
    m_ghostEffects.clear();
    m_pendingDamageEvents.clear();

    // 初始化英雄信息面板：4 种类型
    m_shop.push_back({UnitType::Warrior, 0});
    m_shop.push_back({UnitType::Mage, 0});
    m_shop.push_back({UnitType::Support, 0});
    m_shop.push_back({UnitType::Assassin, 0});

    m_recruitRects.clear();
    m_recruitSlots.clear();
    for (int i = 0; i < 5; ++i)
        m_recruitSlots.push_back({UnitType::Warrior, 0, true});

    refreshRecruitment();

    // 初始化装备掉落
    m_equipDrops.clear();

    // 自动化验证钩子：SYNERA_DEMO_UNITS=1 时在棋盘摆出全职业演示阵容
    if (qEnvironmentVariableIsSet("SYNERA_DEMO_UNITS")) {
        const UnitType types[] = {UnitType::Warrior, UnitType::Mage,
                                  UnitType::Support, UnitType::Assassin};
        for (int i = 0; i < 4; ++i) {
            Unit* h = createUnitFromPool(types[i], true);
            m_board.placeUnit(h, 1 + i * 2, 6);
            Unit* e = createUnitFromPool(types[i], false);
            m_board.placeUnit(e, 1 + i * 2, 1);
        }
        Unit* boss = createUnitFromPool(UnitType::Boss, false, 0, true);
        m_board.placeUnit(boss, 4, 0);
        // 回收槽也放两个演示英雄（备战区立绘验证）
        m_recycleSlots[0] = createUnitFromPool(UnitType::Mage, true, 4);
        m_recycleSlots[1] = createUnitFromPool(UnitType::Warrior, true, 2);
    }
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
    m_dragFromRecycleIndex = -1;
    m_phase = GamePhase::Preparation;
    m_frameCounter = 0;
    m_burnTickCount = 0;
    m_pendingGold = 0;
    m_customBattle = false;
    // 特效已在 endLevel()/initGame() 开头清空，这里无需重复清理

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
        case UnitType::Boss:     return 0;   // Boss 不可招募
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
        newUnit->setMana(newUnit->getMaxMana());
    } else {
        newUnit->resetMana();
    }

    return true;
}

void Synera::checkAutoStarUp()
{
    struct UnitRef {
        Unit* unit;
        bool onBoard;
        int bx, by;
        int slotIdx;
    };

    std::map<std::pair<std::string, int>, std::vector<UnitRef>> groups;

    // 收集棋盘上的英雄
    for (int y = 0; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            Unit* u = m_board.getUnitAt(x, y);
            if (!u || u->isDead() || u->isDisappeared()) continue;
            if (!isHeroSide(u)) continue;
            int fullStar = u->getStarLevel() / 2;
            groups[{u->getName(), fullStar}].push_back({u, true, x, y, -1});
        }
    }

    // 收集回收槽中的英雄
    for (int i = 0; i < 16; ++i) {
        Unit* u = m_recycleSlots[i];
        if (!u || u->isDead() || u->isDisappeared()) continue;
        if (!isHeroSide(u)) continue;
        int fullStar = u->getStarLevel() / 2;
        groups[{u->getName(), fullStar}].push_back({u, false, -1, -1, i});
    }

    bool merged = false;

    for (auto& [key, units] : groups) {
        while (units.size() >= 3) {
            // 取前3个合成
            int newStarLevel = units[0].unit->getStarLevel() + 2;
            if (newStarLevel > 6) newStarLevel = 6;
            UnitType type = units[0].unit->getType();

            // 确认有空回收槽（至少合并中有一个回收槽单位，移除后即空出）
            bool hasRecycleSlot = false;
            for (int i = 0; i < 3; ++i) {
                if (!units[i].onBoard) { hasRecycleSlot = true; break; }
            }
            int emptySlot = findEmptyRecycleSlot();
            if (!hasRecycleSlot && emptySlot < 0) break; // 没有空间放升级单位

            // 收集3个源单位的所有装备
            std::vector<Weapon*> collectedEquips;
            for (int i = 0; i < 3; ++i) {
                Unit* u = units[i].unit;
                for (int ei = 0; ei < static_cast<int>(EquipType::COUNT); ++ei) {
                    EquipType et = static_cast<EquipType>(ei);
                    Weapon* ew = u->getEquip(et);
                    if (ew) {
                        collectedEquips.push_back(ew);
                        u->unequip(et);
                    }
                }
            }

            // 删除3个源单位
            for (int i = 0; i < 3; ++i) {
                auto& ref = units[i];
                if (ref.onBoard)
                    m_board.removeUnit(ref.bx, ref.by);
                else
                    m_recycleSlots[ref.slotIdx] = nullptr;
                ref.unit->setDisappeared(true);
            }

            // 创建升级单位放入回收槽
            Unit* newUnit = createUpgradedHero(type, newStarLevel);

            // 将收集的装备装到升级单位上（每种类型保留一件），多余的放回掉落区
            bool equipped[static_cast<int>(EquipType::COUNT)] = {};
            for (Weapon* ew : collectedEquips) {
                int etIdx = static_cast<int>(ew->getEquipType());
                if (!equipped[etIdx]) {
                    if (newUnit->equip(ew))
                        equipped[etIdx] = true;
                    else
                        m_equipDrops.push_back(ew);
                } else {
                    m_equipDrops.push_back(ew);
                }
            }
            if (emptySlot < 0) {
                // 使用刚释放的回收槽位置
                emptySlot = findEmptyRecycleSlot();
            }
            if (emptySlot >= 0)
                m_recycleSlots[emptySlot] = newUnit;

            if (type == UnitType::Assassin) {
                newUnit->setMana(newUnit->getMaxMana());
            }

            units.erase(units.begin(), units.begin() + 3);
            merged = true;
        }
    }

    if (merged)
        checkAutoStarUp(); // 递归检查是否触发新的合成
}

// ═══════════════════════════════════════════════════════════════
// 开始战斗
// ═══════════════════════════════════════════════════════════════

// 在敌方半场随机放置一个敌方单位；随机 100 次失败后按行扫描兜底
bool Synera::placeEnemyRandom(Unit* eu)
{
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
    return placed;
}

void Synera::startBattle()
{
    auto placeRandom = [this](Unit* eu) { placeEnemyRandom(eu); };

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
    for (int i = 0; i < 5; ++i) m_bondActive[i] = false; // 重置羁绊状态，让 checkAndApplyBonds 正确检测激活
    m_phase = GamePhase::Battle;
    m_frameCounter = 0;
    m_burnTickCount = 0;
}

// ═══════════════════════════════════════════════════════════════
// 关卡结束
// ═══════════════════════════════════════════════════════════════

void Synera::endLevel(bool playerWon)
{
    // 清空战斗中的伤害/治疗显示残留
    m_hitEffects.clear();
    m_slashEffects.clear();
    m_projectileEffects.clear();
    m_healEffects.clear();
    m_ghostEffects.clear();
    m_pendingDamageEvents.clear();

    // 清理所有刺客分身并重置羁绊效果
    removeAssassinClones();
    for (auto& u : m_units) {
        if (!u->isDead() && !u->isDisappeared())
            u->resetBondEffects();
    }
    for (int i = 0; i < 5; ++i) m_bondActive[i] = false;

    // ── 自定义战斗结算：不影响关卡进度 / 玩家 HP，无利息与通关奖励 ──
    if (m_customBattle) {
        if (playerWon) {
            std::vector<Unit*> survivingHeroes = collectSurvivingHeroes();
            for (Unit* u : survivingHeroes)
                u->heal(20);
            retireHeroesToRecycle(survivingHeroes);
            checkAutoStarUp();
            m_gold += m_pendingGold;              // 仅击杀金币（全额）
        } else {
            m_gold += m_pendingGold / 2;           // 击杀金币（半额）
            for (int y = 0; y < Board::SIZE; ++y)
                for (int x = 0; x < Board::SIZE; ++x) {
                    Unit* u = m_board.getUnitAt(x, y);
                    if (u && isEnemySide(u))
                        u->setDisappeared(true);
                }
            retireHeroesToRecycle(collectSurvivingHeroes());
        }
        m_pendingGold = 0;
        m_customBattle = false;
        initLevel();
        return;
    }

    if (playerWon) {
        // 收集场上存活英雄并按星数、类型排序
        std::vector<Unit*> survivingHeroes = collectSurvivingHeroes();

        // 排序：星数高优先，同星按类型：战士>法师>辅助>刺客
        std::sort(survivingHeroes.begin(), survivingHeroes.end(),
            [](Unit* a, Unit* b) {
                if (a->getStarLevel() != b->getStarLevel())
                    return a->getStarLevel() > b->getStarLevel();
                return static_cast<int>(a->getType()) < static_cast<int>(b->getType());
            });

        for (Unit* u : survivingHeroes)
            u->heal(20);
        retireHeroesToRecycle(survivingHeroes);

        checkAutoStarUp();

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
                    && isEnemySide(u))
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
                if (u && isEnemySide(u))
                    u->setDisappeared(true);
            }

        // 场上英雄移回回收槽保留
        retireHeroesToRecycle(collectSurvivingHeroes());

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
            // 多槽装备存档
            QJsonArray equipArr;
            for (int ei = 0; ei < static_cast<int>(EquipType::COUNT); ++ei) {
                Weapon* ew = u->getEquip(static_cast<EquipType>(ei));
                if (ew)
                    equipArr.append(QString::fromStdString(ew->getName()));
                else
                    equipArr.append(QJsonValue::Null);
            }
            bu["equips"] = equipArr;
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
        QJsonArray equipArr;
        for (int ei = 0; ei < static_cast<int>(EquipType::COUNT); ++ei) {
            Weapon* ew = u->getEquip(static_cast<EquipType>(ei));
            if (ew)
                equipArr.append(QString::fromStdString(ew->getName()));
            else
                equipArr.append(QJsonValue::Null);
        }
        ru["equips"] = equipArr;
        recycleArr.append(ru);
    }
    root["recycleUnits"] = recycleArr;

    // 装备掉落
    QJsonArray dropArr;
    for (auto* w : m_equipDrops) {
        if (w)
            dropArr.append(QString::fromStdString(w->getName()));
        else
            dropArr.append(QJsonValue::Null);
    }
    root["equipDrops"] = dropArr;

    QJsonDocument doc(root);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) { // WriteOnly 打开即截断，无需先 remove
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
    m_gold = root["gold"].toInt(8000);
    m_currentLevel = root["currentLevel"].toInt(1);
    m_playerHp = root["playerHp"].toInt(100);
    m_pendingGold = root["pendingGold"].toInt(0);
    m_populationCap = root["populationCap"].toInt(4);

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
        // m_maxHp 由构造函数按 star 设定，无需覆写（覆写会导致装备 HP 重复计算）
        u->resetMana();
        // 先恢复装备（防御装equip()会增加 m_hp），再 setHp 覆写为存档值
        QJsonArray equipArr = o["equips"].toArray();
        for (int ei = 0; ei < equipArr.size() && ei < static_cast<int>(EquipType::COUNT); ++ei) {
            if (equipArr[ei].isNull()) continue;
            std::string ename = equipArr[ei].toString().toStdString();
        Weapon* wp = createWeaponByName(ename);
        if (wp) u->equip(wp);
        }
        // 装备恢复后再设置HP（防御装equip()给m_hp加bonus后，setHp覆写为存档值）
        int savedHp = o["hp"].toInt(-1);
        if (savedHp >= 0) u->setHp(savedHp);
        int mana = o["mana"].toInt(0);
        for (int i = 0; i < mana; ++i) u->gainMana();
        int burning = o["burning"].toInt(0);
        if (burning > 0) u->applyBurning(burning);
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

    // 恢复装备掉落
    m_equipDrops.clear();
    QJsonArray dropArr = root["equipDrops"].toArray();
    for (auto val : dropArr) {
        if (val.isNull()) { m_equipDrops.push_back(nullptr); continue; }
        std::string ename = val.toString().toStdString();
        Weapon* wp = createWeaponByName(ename);
        m_equipDrops.push_back(wp);
    }

    checkAutoStarUp();
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
    renderSlashEffects(painter);
    renderProjectiles(painter);
    renderHealEffects(painter);
    renderGhostEffects(painter);
    renderHeroInfo(painter);
    renderRecruitment(painter);
    renderEquipDrops(painter);
    renderDragGhost(painter);
    renderUI(painter);
    renderBonds(painter);
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

// typeFillColor/typeLabel/unitTypeNameEn 移至 unitvisuals.h，与独立窗口共享
static const char* UNIT_TYPE_NAMES[] = {"Warrior", "Mage", "Support", "Assassin"};

// 统一计算单位装备框宽度：渲染与命中检测共用同一份计算，保证两边永远对齐。
// 装备改用方形图标后宽度为常量（参数保留以兼容调用点签名）。
static int uniformEquipBoxWidth(const Unit* /*u*/, int baseW, int /*textPad*/, bool /*bold*/)
{
    return baseW;
}

int Synera::boardEquipBoxWidth(const Unit* u) const
{
    return uniformEquipBoxWidth(u, 10, 0, true);   // 8px 图标 + 边距
}

int Synera::recycleEquipBoxWidth(const Unit* u) const
{
    return uniformEquipBoxWidth(u, 8, 0, false);   // 7px 图标 + 边距
}

bool Synera::anyHeroOnPlayerHalf() const
{
    for (int y = Board::SIZE / 2; y < Board::SIZE; ++y)
        for (int x = 0; x < Board::SIZE; ++x)
            if (isHeroSide(m_board.getUnitAt(x, y)))
                return true;
    return false;
}

int Synera::findEmptyRecycleSlot() const
{
    for (int i = 0; i < (int)m_recycleSlots.size(); ++i)
        if (m_recycleSlots[i] == nullptr)
            return i;
    return -1;
}

std::vector<Unit*> Synera::collectSurvivingHeroes() const
{
    std::vector<Unit*> heroes;
    for (int y = 0; y < Board::SIZE; ++y)
        for (int x = 0; x < Board::SIZE; ++x) {
            Unit* u = m_board.getUnitAt(x, y);
            if (!u || !isHeroSide(u)) continue;
            if (u->isDead() || u->isDisappeared()) continue;
            heroes.push_back(u);
        }
    return heroes;
}

void Synera::retireHeroesToRecycle(std::vector<Unit*> heroes)
{
    for (Unit* u : heroes) {
        u->resetMoveTimer();
        u->resetAttackTimer();
        int emptySlot = findEmptyRecycleSlot();
        if (emptySlot >= 0) {
            m_recycleSlots[emptySlot] = u;
            m_board.removeUnit(u->getPosition().x, u->getPosition().y);
        } else {
            u->setDisappeared(true);
        }
    }
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

            bool isHero = isHeroSide(unit);
            UnitType t = unit->getType();
            QColor fill = typeFillColor(t, isHero);
            QColor border = isHero ? QColor(100, 170, 255) : QColor(235, 90, 90);

            const QPixmap& portrait = unitPortrait(t);
            if (!portrait.isNull()) {
                // 立绘渲染：暗色底框 + 阵营描边（英雄蓝/敌方红/Boss 金加粗）
                QColor frame = (t == UnitType::Boss) ? QColor(255, 200, 60) : border;
                painter.setBrush(QColor(22, 14, 26));
                painter.setPen(QPen(frame, t == UnitType::Boss ? 2 : 1));
                painter.drawRoundedRect(ur, 6, 6);
                painter.save();
                painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
                painter.drawPixmap(ur.adjusted(2, 2, -2, -2), portrait);
                painter.restore();
            } else {
                painter.setBrush(fill);
                painter.setPen(QPen(border, 1));
                painter.drawRoundedRect(ur, 6, 6);

                // 角色标签
                painter.setPen(Qt::white);
                QFont typeFont;
                typeFont.setPixelSize(20);
                typeFont.setBold(true);
                painter.setFont(typeFont);
                painter.drawText(ur, Qt::AlignCenter, typeLabel(t));
            }

            // 名字（顶部）
            QFont nameFont;
            nameFont.setPixelSize(9);
            nameFont.setBold(true);
            painter.setFont(nameFont);
            painter.drawText(ur.adjusted(3, 2, 0, 0),
                             QString::fromStdString(unit->getName()));

            // HP 条（底部）
            int barH = 6;
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
            hpFont.setPixelSize(7);
            painter.setFont(hpFont);
            painter.setPen(Qt::white);
            painter.drawText(barBg, Qt::AlignCenter,
                             QString("%1/%2").arg(unit->getHp()).arg(unit->getMaxHp()));

            // 法力值条
            int manaBarH = 4;
            int manaBarY = barBg.top() - manaBarH - 2;
            int manaMax = unit->getMaxMana();
            double manaRatio = manaMax > 0 ? (double)unit->getMana() / manaMax : 0;
            if (manaRatio > 1.0) manaRatio = 1.0;
            QRect manaBarBg(ur.left() + 2, manaBarY, ur.width() - 4, manaBarH);
            painter.setBrush(QColor(15, 15, 35));
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(manaBarBg, 1, 1);

            QRect manaBarFill(manaBarBg.left(), manaBarY, (int)(manaBarBg.width() * manaRatio), manaBarH);
            painter.setBrush(QColor(50, 120, 240));
            painter.drawRoundedRect(manaBarFill, 1, 1);

            // 装备框（右侧5格，图标显示）
            if (isHero) {
                int boxH = 10, boxGap = 1;
                int boxStartY = rc.top() + 1;
                int maxSlots = unit->getMaxEquipSlots();
                int maxBoxW = boardEquipBoxWidth(unit);

                for (int ei = 0; ei < static_cast<int>(EquipType::COUNT); ++ei) {
                    EquipType et = static_cast<EquipType>(ei);
                    Weapon* ew = unit->getEquip(et);
                    if (!ew && ei >= maxSlots) continue;

                    QRect boxRect(rc.right() - maxBoxW - 2, boxStartY + ei * (boxH + boxGap), maxBoxW, boxH);

                    if (ew) {
                        painter.setBrush(QColor(40, 40, 55));
                        painter.setPen(QPen(QColor(255, 210, 50), 1));
                        painter.drawRoundedRect(boxRect, 2, 2);
                        const QPixmap& icon = equipIcon(ew->getIconFile(), 8);
                        painter.drawPixmap(boxRect.center() - QPoint(icon.width() / 2, icon.height() / 2), icon);
                    } else {
                        painter.setBrush(QColor(28, 28, 38));
                        painter.setPen(QPen(QColor(60, 60, 75), 1));
                        painter.drawRoundedRect(boxRect, 2, 2);
                    }
                }
            }

            // 星数图标（仅英雄显示，在格子左侧）
            if (isHero) {
                double starR = 4.0;
                double starSpacing = 12.0;
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

            // 燃烧特效：身上持续冒红色小火苗/泡泡（循环上浮淡出）
            if (unit->isBurning()) {
                painter.save();
                unsigned seed = (unsigned)(x * 73856093u) ^ (unsigned)(y * 19349663u);
                for (int i = 0; i < 3; ++i) {
                    seed = seed * 1664525u + 1013904223u;
                    int wob = (int)((seed >> 12) % 9) - 4;            // 水平摆动
                    double phase = ((m_frameCounter + i * 8 + ((seed >> 6) % 24)) % 24) / 24.0;
                    double fx = ur.center().x() + (i - 1) * 9 + wob * (0.3 + phase);
                    double fy = ur.bottom() - 6 - phase * (ur.height() * 0.55);
                    double fr = 2.6 + 1.6 * std::sin(phase * 3.14159);
                    int a = (int)(200 * std::sin(phase * 3.14159));   // 上升过程先亮后暗
                    if (a <= 0) continue;
                    QRadialGradient fg(QPointF(fx, fy), fr + 1.5);
                    fg.setColorAt(0.0, QColor(255, 230, 120, a));
                    fg.setColorAt(0.55, QColor(255, 100, 40, a));
                    fg.setColorAt(1.0, QColor(200, 30, 20, 0));
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(fg);
                    painter.drawEllipse(QPointF(fx, fy), fr + 1.5, fr + 1.5);
                }
                painter.restore();
            }

            // 伤害/治疗浮动文本（右上角）
            int hitOffsetY = 0;
            for (auto& he : m_hitEffects) {
                if (he.cellX != x || he.cellY != y) continue;
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
// 战士/刺客击打特效（攻击者与目标之间：刀光弧 + 命中闪光 + 火花）
// kind: 0=战士普攻 1=战士技能重斩(双弧更大更亮) 2=刺客快速斩击
// ═══════════════════════════════════════════════════════════════

void Synera::renderSlashEffects(QPainter& painter)
{
    const double PI = 3.14159265358979323846;

    for (const SlashEffect& e : m_slashEffects) {
        int elapsed = m_frameCounter - e.startFrame;
        if (elapsed < 0 || e.duration <= 0) continue;
        double progress = (double)elapsed / e.duration;
        if (progress >= 1.0) continue;

        QPointF from = cellRect(e.fromX, e.fromY).center();
        QPointF to   = cellRect(e.cellX, e.cellY).center();
        QPointF mid((from.x() + to.x()) / 2.0, (from.y() + to.y()) / 2.0);

        double angle = std::atan2(to.y() - from.y(), to.x() - from.x());
        double alpha = 1.0 - progress;

        // 按类型调整视觉强度
        double radiusScale = 0.62;
        double glowW = 6, coreW = 2;
        int sparkCount = 5;
        QColor glowColor(255, 160, 40);
        QColor coreColor(255, 255, 255);
        bool doubleArc = false;
        if (e.kind == 1) {          // 战士技能重斩：更大更亮
            radiusScale = 0.88;
            glowW = 10; coreW = 4;
            sparkCount = 8;
            glowColor = QColor(255, 210, 60);
            doubleArc = true;
        } else if (e.kind == 2) {   // 刺客快速斩击：更短促锋锐
            radiusScale = 0.50;
            glowW = 4; coreW = 2;
            sparkCount = 3;
            glowColor = QColor(255, 230, 120);
        }

        // 命中点：目标中心向攻击者方向回退 25%，特效打在两格交界处
        QPointF hit(to.x() - std::cos(angle) * CELL_SIZE * 0.25,
                    to.y() - std::sin(angle) * CELL_SIZE * 0.25);

        painter.save();

        // ── 刀光弧：横跨两个战士之间，随帧展开并淡出 ──
        {
            double radius = CELL_SIZE * radiusScale;
            double sweep  = 0.35 + 0.85 * progress;              // 挥砍展开角
            double side   = (e.startFrame % 2 == 0) ? 1.0 : -1.0; // 交替挥砍方向
            double base   = angle + PI / 2.0 * side;

            auto drawOneArc = [&](double centerAngle) {
                double a0 = centerAngle - sweep * side;
                double a1 = centerAngle + sweep * side;
                QPainterPath arc;
                arc.moveTo(mid.x() + radius * std::cos(a0), mid.y() + radius * std::sin(a0));
                arc.quadTo(mid.x(), mid.y(),
                           mid.x() + radius * std::cos(a1), mid.y() + radius * std::sin(a1));
                painter.setPen(QPen(QColor(glowColor.red(), glowColor.green(), glowColor.blue(),
                                           (int)(160 * alpha)), glowW));
                painter.drawPath(arc);
                painter.setPen(QPen(QColor(coreColor.red(), coreColor.green(), coreColor.blue(),
                                           (int)(230 * alpha)), coreW));
                painter.drawPath(arc);
            };

            drawOneArc(base);
            if (doubleArc) drawOneArc(base + PI);   // 技能重斩：对侧再来一道，成交叉斩
        }

        // ── 命中闪光 ──
        {
            double flashScale = (e.kind == 1) ? 1.6 : 1.0;
            double r = (3.0 + 7.0 * progress) * flashScale;
            QRadialGradient grad(hit, r);
            grad.setColorAt(0.0, QColor(255, 255, 220, (int)(220 * alpha)));
            grad.setColorAt(1.0, QColor(255, 200, 80, 0));
            painter.setPen(Qt::NoPen);
            painter.setBrush(grad);
            painter.drawEllipse(hit, r, r);
        }

        // ── 火花放射线：从命中点沿攻击反方向扇形溅射 ──
        {
            unsigned seed = (unsigned)(e.cellX * 73856093u)
                          ^ (unsigned)(e.cellY * 19349663u)
                          ^ (unsigned)(e.startFrame * 83492791u);
            for (int i = 0; i < sparkCount; ++i) {
                seed = seed * 1664525u + 1013904223u;
                double jitter = ((seed >> 16) % 1000) / 1000.0 - 0.5;
                double theta = angle + PI + jitter * 2.4;

                double startDist = 4.0 + 15.0 * progress;
                double len = 5.0 + 9.0 * progress;
                QPointF sp(hit.x() + std::cos(theta) * startDist,
                           hit.y() + std::sin(theta) * startDist);
                QPointF ep(sp.x() + std::cos(theta) * len,
                           sp.y() + std::sin(theta) * len);

                painter.setPen(QPen(QColor(255, 255 - (int)(140 * progress), 60,
                                           (int)(220 * alpha)), 2));
                painter.drawLine(sp, ep);
            }
        }

        // ── 技能重斩追加：命中点冲击环 ──
        if (e.kind == 1) {
            double r = CELL_SIZE * (0.25 + 0.55 * progress);
            painter.setPen(QPen(QColor(255, 230, 120, (int)(180 * alpha)), 3));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(hit, r, r * 0.6);
        }

        painter.restore();
    }
}

// ═══════════════════════════════════════════════════════════════
// 法师火球弹道特效（抛物线飞行的小火球 + 命中爆裂）
// ═══════════════════════════════════════════════════════════════

void Synera::renderProjectiles(QPainter& painter)
{
    const double FLIGHT_END = 0.78;   // 前 78% 帧飞行，之后爆裂

    for (const ProjectileEffect& e : m_projectileEffects) {
        int elapsed = m_frameCounter - e.startFrame;
        if (elapsed < 0 || e.duration <= 0) continue;
        double progress = (double)elapsed / e.duration;
        if (progress >= 1.0) continue;

        QPointF from = cellRect(e.fromX, e.fromY).center();
        QPointF to   = cellRect(e.toX, e.toY).center();

        painter.save();

        if (progress < FLIGHT_END) {
            // ── 飞行段：沿弧线（略微上抛）飞向目标 ──
            double t = progress / FLIGHT_END;
            QPointF mid((from.x() + to.x()) / 2.0, (from.y() + to.y()) / 2.0);
            QPointF ctrl(mid.x(), mid.y() - 14.0);   // 控制点上抬，形成小弧线

            // 二次贝塞尔求当前位置
            double u = 1.0 - t;
            QPointF pos(u * u * from.x() + 2 * u * t * ctrl.x() + t * t * to.x(),
                        u * u * from.y() + 2 * u * t * ctrl.y() + t * t * to.y());

            // 尾迹：沿轨迹回退的 3 个渐隐火点
            for (int i = 1; i <= 3; ++i) {
                double tt = t - i * 0.07;
                if (tt < 0.0) continue;
                double uu = 1.0 - tt;
                QPointF tp(uu * uu * from.x() + 2 * uu * tt * ctrl.x() + tt * tt * to.x(),
                           uu * uu * from.y() + 2 * uu * tt * ctrl.y() + tt * tt * to.y());
                int tr = 4 - i;
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(255, 120, 40, 120 - i * 35));
                painter.drawEllipse(tp, tr, tr);
            }

            // 火球本体：外焰红 + 内核亮黄白
            QRadialGradient grad(pos, 6.0);
            grad.setColorAt(0.0, QColor(255, 255, 200, 255));
            grad.setColorAt(0.45, QColor(255, 200, 60, 235));
            grad.setColorAt(1.0, QColor(255, 90, 20, 120));
            painter.setPen(Qt::NoPen);
            painter.setBrush(grad);
            painter.drawEllipse(pos, 6.0, 6.0);
        } else {
            // ── 爆裂段：目标处橙色冲击环 + 碎火星 ──
            double t = (progress - FLIGHT_END) / (1.0 - FLIGHT_END);
            double alpha = 1.0 - t;

            painter.setPen(QPen(QColor(255, 170, 50, (int)(200 * alpha)), 3));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(to, 4.0 + 12.0 * t, 4.0 + 12.0 * t);

            unsigned seed = (unsigned)(e.toX * 73856093u) ^ (unsigned)(e.toY * 19349663u)
                          ^ (unsigned)(e.startFrame * 83492791u);
            for (int i = 0; i < 5; ++i) {
                seed = seed * 1664525u + 1013904223u;
                double theta = ((seed >> 8) % 628) / 100.0;   // 0~6.28
                double dist = 6.0 + 14.0 * t;
                painter.setPen(QPen(QColor(255, 200 - (int)(100 * t), 60,
                                           (int)(220 * alpha)), 2));
                painter.drawLine(QPointF(to.x() + std::cos(theta) * dist,
                                         to.y() + std::sin(theta) * dist),
                                 QPointF(to.x() + std::cos(theta) * (dist + 5.0),
                                         to.y() + std::sin(theta) * (dist + 5.0)));
            }
        }

        painter.restore();
    }
}

// ═══════════════════════════════════════════════════════════════
// 辅助治疗特效（大 "+" 上浮 + 环绕小 "+" 粒子）
// ═══════════════════════════════════════════════════════════════

void Synera::renderHealEffects(QPainter& painter)
{
    for (const HealEffect& e : m_healEffects) {
        int elapsed = m_frameCounter - e.startFrame;
        if (elapsed < 0 || e.duration <= 0) continue;
        double progress = (double)elapsed / e.duration;
        if (progress >= 1.0) continue;

        QRect rc = cellRect(e.cellX, e.cellY);
        double alpha = 1.0 - progress;
        double rise = 10.0 * progress;   // 整体上浮

        painter.save();
        QFont bigFont;
        bigFont.setPixelSize(e.isSkill ? 22 : 14);
        bigFont.setBold(true);
        QFont smallFont;
        smallFont.setPixelSize(e.isSkill ? 11 : 8);
        smallFont.setBold(true);

        // 大 "+"：单位中上方，随时间上浮淡出
        painter.setFont(bigFont);
        painter.setPen(QColor(90, 255, 110, (int)(235 * alpha)));
        QRect bigRect(rc.left(), rc.top() - 18 - (int)rise, rc.width(), 22);
        painter.drawText(bigRect, Qt::AlignHCenter | Qt::AlignVCenter, QString("+"));

        // 2~3 个小 "+"：围绕大 "+" 向四周飘散
        unsigned seed = (unsigned)(e.cellX * 19349663u) ^ (unsigned)(e.cellY * 73856093u)
                      ^ (unsigned)(e.startFrame * 83492791u);
        int smallCount = e.isSkill ? 3 : 2;
        painter.setFont(smallFont);
        for (int i = 0; i < smallCount; ++i) {
            seed = seed * 1664525u + 1013904223u;
            double theta = ((seed >> 8) % 628) / 100.0;   // 0~6.28
            double dist = 8.0 + 16.0 * progress;
            int sx = (int)(rc.center().x() + std::cos(theta) * dist);
            int sy = (int)(rc.center().y() + std::sin(theta) * dist * 0.6 - rise);
            painter.setPen(QColor(120, 255, 140, (int)(200 * alpha)));
            painter.drawText(QRect(sx - 6, sy - 6, 12, 12),
                             Qt::AlignCenter, QString("+"));
        }

        painter.restore();
    }
}

// ═══════════════════════════════════════════════════════════════
// 刺客瞬移残影特效（高透明度幻影沿瞬移路径排布并淡出）
// ═══════════════════════════════════════════════════════════════

void Synera::renderGhostEffects(QPainter& painter)
{
    const double GHOST_MAX_ALPHA = 0.32;   // 高透明度（约 1/3 不透明度）

    for (const GhostEffect& e : m_ghostEffects) {
        int elapsed = m_frameCounter - e.startFrame;
        if (elapsed < 0 || e.duration <= 0) continue;
        double progress = (double)elapsed / e.duration;
        if (progress >= 1.0) continue;

        QPointF from = cellRect(e.fromX, e.fromY).center();
        QPointF to   = cellRect(e.toX, e.toY).center();

        QColor fill = typeFillColor(static_cast<UnitType>(e.type), e.isHero);
        QColor border = e.isHero ? QColor(100, 170, 255) : QColor(235, 90, 90);

        painter.save();

        // 3 个幻影位于路径 25% / 50% / 75% 处，随时间整体淡出
        const double fractions[3] = {0.25, 0.5, 0.75};
        for (int i = 0; i < 3; ++i) {
            double fx = from.x() + (to.x() - from.x()) * fractions[i];
            double fy = from.y() + (to.y() - from.y()) * fractions[i];

            // 越靠后的残影越淡
            double localAlpha = GHOST_MAX_ALPHA * (1.0 - progress) * (1.0 - 0.25 * i);
            int m = 8;
            QRect gr((int)fx - CELL_SIZE / 2 + m, (int)fy - CELL_SIZE / 2 + m,
                     CELL_SIZE - 2 * m, CELL_SIZE - 2 * m);

            painter.setOpacity(localAlpha);
            painter.setBrush(fill);
            painter.setPen(QPen(border, 1));
            painter.drawRoundedRect(gr, 6, 6);
        }

        // 瞬移轨迹虚线
        painter.setOpacity(GHOST_MAX_ALPHA * 0.8 * (1.0 - progress));
        QPen dashPen(QColor(220, 200, 255), 1, Qt::DashLine);
        painter.setPen(dashPen);
        painter.drawLine(from, to);

        painter.restore();
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
    titleFont.setPixelSize(11);
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

        // 职业立绘图块
        QRect colorRect(panelRect.left() + 4, panelRect.top() + 4, 22, 22);
        drawUnitChip(painter, colorRect, slot.type, true, 3);

        // 名称
        QFont nameFont;
        nameFont.setPixelSize(8);
        nameFont.setBold(true);
        painter.setFont(nameFont);
        painter.setPen(QColor(200, 200, 220));
        painter.drawText(colorRect.right() + 6, panelRect.top() + 12, UNIT_TYPE_NAMES[(int)slot.type]);

        // 属性数据（数值直接引用各类的 BASE 常量，避免与实际数值漂移）
        QFont statFont;
        statFont.setPixelSize(7);
        painter.setFont(statFont);
        painter.setPen(QColor(170, 170, 190));
        int sx = panelRect.left() + 6;
        int sy = panelRect.top() + 24;
        int lh = 10;
        int baseCost = heroCost(slot.type);
        switch (slot.type) {
            case UnitType::Boss:       // 面板不展示 Boss，防御性处理
                break;
            case UnitType::Warrior:
                painter.drawText(sx, sy,      QString("HP:%1  ATK:%2").arg(WarriorHero::BASE_HP).arg(WarriorHero::BASE_ATK));
                painter.drawText(sx, sy + lh, "Rng:1  Spd:60/120");
                break;
            case UnitType::Mage:
                painter.drawText(sx, sy,      QString("HP:%1  ATK:%2").arg(MageHero::BASE_HP).arg(MageHero::BASE_ATK));
                painter.drawText(sx, sy + lh, "Rng:4  Spd:60/120");
                break;
            case UnitType::Support:
                painter.drawText(sx, sy,      QString("HP:%1  Heal:%2").arg(SupportHero::BASE_HP).arg(SupportHero::BASE_HEAL));
                painter.drawText(sx, sy + lh, "Rng:2  Spd:60/120");
                break;
            case UnitType::Assassin:
                painter.drawText(sx, sy,      QString("HP:%1  ATK:%2").arg(AssassinHero::BASE_HP).arg(AssassinHero::BASE_ATK));
                painter.drawText(sx, sy + lh, "Rng:1  Spd:60/80");
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
    titleFont.setPixelSize(11);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(QColor(180, 180, 200));
    painter.drawText(LEFT_PANEL_X, titleY, "Recruit");

    // 刷新按钮（标题右侧）
    int refreshW = 56, refreshH = 16;
    QRect refreshRect(LEFT_PANEL_X + LEFT_PANEL_W - refreshW, titleY - 14, refreshW, refreshH);
    m_refreshButtonRect = refreshRect;

    bool canRefresh = (m_gold >= 15);
    painter.setBrush(canRefresh ? QColor(55, 110, 55) : QColor(55, 55, 55));
    painter.setPen(QPen(canRefresh ? QColor(80, 180, 80) : QColor(90, 90, 90), 1));
    painter.drawRoundedRect(refreshRect, 3, 3);

    painter.setPen(canRefresh ? Qt::white : QColor(150, 150, 150));
    QFont rfFont;
    rfFont.setPixelSize(8);
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
            emptyFont.setPixelSize(9);
            painter.setFont(emptyFont);
            painter.drawText(rc, Qt::AlignCenter, "- empty -");
        } else {
            painter.setBrush(QColor(38, 38, 48));
            painter.setPen(QPen(QColor(70, 70, 85), 1));
            painter.drawRoundedRect(rc, 4, 4);

            // 职业立绘图块
            QRect colorRect(rc.left() + 4, rc.top() + 3, 26, rc.height() - 6);
            drawUnitChip(painter, colorRect, slot.type, true, 4);

            // 名称
            QFont nameFont;
            nameFont.setPixelSize(8);
            nameFont.setBold(true);
            painter.setFont(nameFont);
            painter.setPen(QColor(200, 200, 220));
            painter.drawText(colorRect.right() + 5, rc.top() + 12, UNIT_TYPE_NAMES[(int)slot.type]);

            // 价格（右侧）
            bool canBuy = (m_gold >= slot.price);
            painter.setPen(canBuy ? QColor(80, 220, 80) : QColor(200, 80, 80));
            QFont priceFont;
            priceFont.setPixelSize(9);
            priceFont.setBold(true);
            painter.setFont(priceFont);
            QRect priceRect(rc.right() - 55, rc.top(), 50, rc.height());
            painter.drawText(priceRect, Qt::AlignRight | Qt::AlignVCenter, QString("$%1").arg(slot.price));
        }
    }

    // 人口上限升级按钮（招募区下方）
    int popBtnY = RECRUIT_START_Y + 5 * (RECRUIT_SLOT_H + RECRUIT_SPACING) + 8;
    int popCost = 100 * (m_populationCap - 4);
    QRect popBtnRect(LEFT_PANEL_X, popBtnY, 72, 22);
    m_popUpgradeButtonRect = popBtnRect;

    bool canBuyPop = (m_gold >= popCost);
    painter.setBrush(canBuyPop ? QColor(45, 80, 130) : QColor(55, 55, 55));
    painter.setPen(QPen(canBuyPop ? QColor(80, 150, 220) : QColor(90, 90, 90), 1));
    painter.drawRoundedRect(popBtnRect, 4, 4);

    painter.setPen(canBuyPop ? Qt::white : QColor(150, 150, 150));
    QFont popBtnFont;
    popBtnFont.setPixelSize(9);
    popBtnFont.setBold(true);
    painter.setFont(popBtnFont);
    painter.drawText(popBtnRect, Qt::AlignCenter, QString("Pop+ $%1").arg(popCost));

    // 装备合成树按钮（人口升级按钮下方）
    int synthBtnY = popBtnRect.bottom() + 8;
    QRect synthBtnRect(LEFT_PANEL_X, synthBtnY, LEFT_PANEL_W, 22);
    m_synthTreeButtonRect = synthBtnRect;

    painter.setBrush(QColor(55, 55, 70));
    painter.setPen(QPen(QColor(255, 210, 60), 1));
    painter.drawRoundedRect(synthBtnRect, 4, 4);

    painter.setPen(QColor(255, 210, 60));
    QFont synthBtnFont;
    synthBtnFont.setPixelSize(9);
    synthBtnFont.setBold(true);
    painter.setFont(synthBtnFont);
    painter.drawText(synthBtnRect, Qt::AlignCenter, QString::fromUtf8("\350\243\205\345\244\207\345\220\210\346\210\220\346\240\221")); // 装备合成树

    // 自定义难度按钮（装备合成树按钮下方）
    int customBtnY = synthBtnRect.bottom() + 8;
    QRect customBtnRect(LEFT_PANEL_X, customBtnY, LEFT_PANEL_W, 22);
    m_customButtonRect = customBtnRect;

    bool canCustom = (m_phase == GamePhase::Preparation && !m_gameOver);
    painter.setBrush(canCustom ? QColor(70, 45, 80) : QColor(50, 50, 58));
    painter.setPen(QPen(canCustom ? QColor(190, 110, 230) : QColor(90, 90, 95), 1));
    painter.drawRoundedRect(customBtnRect, 4, 4);

    painter.setPen(canCustom ? QColor(220, 170, 250) : QColor(140, 140, 145));
    QFont customBtnFont;
    customBtnFont.setPixelSize(9);
    customBtnFont.setBold(true);
    painter.setFont(customBtnFont);
    painter.drawText(customBtnRect, Qt::AlignCenter, QString::fromUtf8("自定义难度"));

}

// ═══════════════════════════════════════════════════════════════
// 回收槽 (棋盘下方)
// ═══════════════════════════════════════════════════════════════

QRect Synera::recycleSlotRect(int row, int col) const
{
    return QRect(RECYCLE_START_X + col * (RECYCLE_SLOT_W + RECYCLE_SPACING),
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
    int startX = RECYCLE_START_X;

    // 标题常驻
    QFont titleFont;
    titleFont.setPixelSize(10);
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
                // 有单位：职业立绘图块（HP/星数/装备照常叠加）
                UnitType t = u->getType();
                drawUnitChip(painter, rc, t, true, 4);

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
                double starR = 3.0;
                double starSpacing = 7.0;
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

                // 装备图标
                int maxSlots = u->getMaxEquipSlots();
                int eqBoxH = 8, eqBoxGap = 1;
                int eqBoxStartY = rc.top() + 2;
                int maxEqBoxW = recycleEquipBoxWidth(u);

                for (int ei = 0; ei < static_cast<int>(EquipType::COUNT); ++ei) {
                    EquipType et = static_cast<EquipType>(ei);
                    Weapon* ew = u->getEquip(et);
                    if (!ew && ei >= maxSlots) continue;

                    QRect eqBox(rc.right() - maxEqBoxW - 2, eqBoxStartY + ei * (eqBoxH + eqBoxGap), maxEqBoxW, eqBoxH);
                    if (ew) {
                        painter.setBrush(QColor(40, 40, 55));
                        painter.setPen(QPen(QColor(255, 210, 50), 1));
                        painter.drawRoundedRect(eqBox, 1, 1);
                        const QPixmap& icon = equipIcon(ew->getIconFile(), 7);
                        painter.drawPixmap(eqBox.center() - QPoint(icon.width() / 2, icon.height() / 2), icon);
                    } else {
                        painter.setBrush(QColor(28, 28, 38));
                        painter.setPen(QPen(QColor(60, 60, 75), 1));
                        painter.drawRoundedRect(eqBox, 1, 1);
                    }
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
// 装备掉落 (回收槽下方)
// ═══════════════════════════════════════════════════════════════

void Synera::tryEquipDrop()
{
    // 去掉每关获取上限，仅限制掉落区容量
    if ((int)m_equipDrops.size() >= MAX_EQUIP_DROPS) return;
    int chance = 10 * m_currentLevel + 5; // 基础概率 +5%
    if (chance > 100) chance = 100;
    if ((std::rand() % 100) >= chance) return;

    // 随机选一种基础装备
    int r = std::rand() % 5;
    std::unique_ptr<Weapon> w;
    switch (r) {
        case 0: w = std::make_unique<BasicAttackWeapon>(); break;
        case 1: w = std::make_unique<BasicDefenseWeapon>(); break;
        case 2: w = std::make_unique<BasicSpeedWeapon>(); break;
        case 3: w = std::make_unique<BasicManaWeapon>(); break;
        case 4: w = std::make_unique<BasicRangeWeapon>(); break;
    }
    Weapon* wp = w.get();
    m_weapons.push_back(std::move(w));
    m_equipDrops.push_back(wp);
}

void Synera::renderEquipDrops(QPainter& painter)
{
    int startX = RECYCLE_START_X;
    int dropY = RECYCLE_Y + 2 * (RECYCLE_SLOT_H + 8) + 8;

    // 标题
    QFont titleFont;
    titleFont.setPixelSize(10);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(QColor(160, 160, 180));
    painter.drawText(startX, dropY - 2, "Equipment Drops");

    m_equipDropRects.clear();
    int slotW = 38, slotH = 24, slotGap = 3;

    for (int i = 0; i < MAX_EQUIP_DROPS; ++i) {
        QRect rc(startX + i * (slotW + slotGap), dropY + 8, slotW, slotH);
        m_equipDropRects.push_back(rc);

        if (i < (int)m_equipDrops.size() && m_equipDrops[i]) {
            // 有装备掉落：图标显示
            painter.setBrush(QColor(45, 40, 55));
            painter.setPen(QPen(QColor(255, 180, 50), 1));
            painter.drawRoundedRect(rc, 3, 3);

            const QPixmap& icon = equipIcon(m_equipDrops[i]->getIconFile(), 18);
            painter.drawPixmap(rc.center() - QPoint(icon.width() / 2, icon.height() / 2), icon);
        } else {
            // 空槽
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(QColor(255, 255, 255, 40), 1));
            painter.drawRoundedRect(rc, 3, 3);
        }
    }

    // 拖拽中的装备提示
    if (m_draggedWeapon) {
        painter.setPen(QColor(255, 210, 80));
        QFont hintFont;
        hintFont.setPixelSize(8);
        hintFont.setBold(true);
        painter.setFont(hintFont);
        QString hint = QString("Drag onto hero to equip: %1")
            .arg(QString::fromStdString(m_draggedWeapon->getDisplayName()));
        painter.drawText(startX, dropY + slotH + 22, hint);
    }

    // 合成提示
    painter.setPen(QColor(180, 180, 100));
    QFont synthNoteFont;
    synthNoteFont.setPixelSize(7);
    synthNoteFont.setBold(true);
    painter.setFont(synthNoteFont);
    painter.drawText(startX, dropY + slotH + 20, QString::fromUtf8("\346\257\217\346\254\241\345\215\207\347\272\247\350\243\205\345\244\207\351\234\200\350\246\201\350\200\227\350\264\271300\351\207\221\345\270\201")); // 每次升级装备需要耗费300金币
}

int Synera::findEquipDropAt(const QPoint& pixel) const
{
    for (int i = 0; i < (int)m_equipDropRects.size(); ++i)
        if (m_equipDropRects[i].contains(pixel))
            return i;
    return -1;
}

Unit* Synera::findBoardEquipSlotAt(const QPoint& pixel, EquipType& outType) const
{
    for (int y = 0; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            Unit* u = m_board.getUnitAt(x, y);
            if (!u || u->isDead() || u->isDisappeared()) continue;
            if (!isHeroSide(u)) continue;
            QRect rc = cellRect(x, y);
            int boxH = 10, boxGap = 1;
            int boxStartY = rc.top() + 1;
            int maxSlots = u->getMaxEquipSlots();
            int maxBoxW = boardEquipBoxWidth(u); // 与渲染共用同一计算
            if (pixel.x() < rc.right() - maxBoxW - 2 || pixel.x() > rc.right()) continue;
            for (int ei = 0; ei < static_cast<int>(EquipType::COUNT); ++ei) {
                EquipType et = static_cast<EquipType>(ei);
                Weapon* ew = u->getEquip(et);
                if (!ew && ei >= maxSlots) continue;
                QRect boxRect(rc.right() - maxBoxW - 2, boxStartY + ei * (boxH + boxGap), maxBoxW, boxH);
                if (boxRect.contains(pixel)) {
                    outType = et;
                    return u;
                }
            }
        }
    }
    return nullptr;
}

Unit* Synera::findRecycleEquipSlotAt(const QPoint& pixel, EquipType& outType) const
{
    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 8; ++col) {
            int idx = row * 8 + col;
            Unit* u = m_recycleSlots[idx];
            if (!u || u->isDead() || u->isDisappeared()) continue;
            if (!isHeroSide(u)) continue;
            QRect rc = recycleSlotRect(row, col);
            int eqBoxH = 8, eqBoxGap = 1;
            int eqBoxStartY = rc.top() + 2;
            int maxSlots = u->getMaxEquipSlots();
            int maxEqBoxW = recycleEquipBoxWidth(u); // 与渲染共用同一计算
            if (pixel.x() < rc.right() - maxEqBoxW - 2 || pixel.x() > rc.right()) continue;
            for (int ei = 0; ei < static_cast<int>(EquipType::COUNT); ++ei) {
                EquipType et = static_cast<EquipType>(ei);
                Weapon* ew = u->getEquip(et);
                if (!ew && ei >= maxSlots) continue;
                QRect eqBox(rc.right() - maxEqBoxW - 2, eqBoxStartY + ei * (eqBoxH + eqBoxGap), maxEqBoxW, eqBoxH);
                if (eqBox.contains(pixel)) {
                    outType = et;
                    return u;
                }
            }
        }
    }
    return nullptr;
}

// ═══════════════════════════════════════════════════════════════
// 拖拽幽灵
// ═══════════════════════════════════════════════════════════════

void Synera::renderDragGhost(QPainter& painter)
{
    if (m_draggedUnit) {
        QRect unitRect(m_dragCurrentPos.x() - CELL_SIZE / 2,
                       m_dragCurrentPos.y() - CELL_SIZE / 2,
                       CELL_SIZE, CELL_SIZE);

        painter.save();
        painter.setOpacity(0.75);

        bool isHero = isHeroSide(m_draggedUnit);
        UnitType t = m_draggedUnit->getType();
        const QPixmap& portrait = unitPortrait(t);
        if (!portrait.isNull()) {
            // 立绘幽灵：暗色底框 + 黄边 + 贴图（保持整体 0.75 透明度）
            painter.setBrush(QColor(22, 14, 26));
            painter.setPen(QPen(QColor(255, 255, 100), 2));
            painter.drawRoundedRect(unitRect, 8, 8);
            painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
            painter.drawPixmap(unitRect.adjusted(3, 3, -3, -3), portrait);
        } else {
            painter.setBrush(typeFillColor(t, isHero));
            painter.setPen(QPen(QColor(255, 255, 100), 2));
            painter.drawRoundedRect(unitRect, 8, 8);
        }

        if (portrait.isNull()) {
            // 无立绘时才画单字职业标签，避免盖在贴图上
            painter.setPen(Qt::white);
            QFont f;
            f.setPixelSize(18);
            f.setBold(true);
            painter.setFont(f);
            painter.drawText(unitRect, Qt::AlignCenter, typeLabel(t));
        }

        painter.restore();
    }

    if (m_draggedWeapon) {
        int gw = 48, gh = 34;
        QRect gearRect(m_dragCurrentPos.x() - gw / 2,
                       m_dragCurrentPos.y() - gh / 2, gw, gh);
        painter.save();
        painter.setOpacity(0.8);
        painter.setBrush(QColor(45, 40, 60));
        painter.setPen(QPen(QColor(255, 210, 50), 2));
        painter.drawRoundedRect(gearRect, 4, 4);

        const QPixmap& icon = equipIcon(m_draggedWeapon->getIconFile(), 18);
        painter.drawPixmap(gearRect.center().x() - icon.width() / 2,
                           gearRect.top() + 2, icon);

        painter.setPen(QColor(255, 220, 80));
        QFont gf;
        gf.setPixelSize(8);
        gf.setBold(true);
        painter.setFont(gf);
        QRect nameRect(gearRect.left(), gearRect.bottom() - 12, gw, 12);
        painter.drawText(nameRect, Qt::AlignHCenter | Qt::AlignVCenter,
                         QString::fromStdString(m_draggedWeapon->getDisplayName()));
        painter.restore();
    }
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
    infoFont.setPixelSize(15);
    infoFont.setBold(true);
    painter.setFont(infoFont);

    // HP - 棋盘左上方
    QColor hpColor = m_playerHp > 30 ? QColor(80, 220, 80) : QColor(220, 60, 60);
    painter.setPen(hpColor);
    painter.drawText(BOARD_OFFSET_X, infoY, QString("HP: %1").arg(m_playerHp));

    // 关卡 - 棋盘上方中间
    painter.setPen(QColor(255, 210, 80));
    QFont lvlFont;
    lvlFont.setPixelSize(16);
    lvlFont.setBold(true);
    painter.setFont(lvlFont);
    QString lvlText = QString("Level %1 / %2").arg(m_currentLevel).arg(MAX_LEVEL);
    if (m_customBattle)
        lvlText += QString::fromUtf8(" (自定义)");
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
    popFont.setPixelSize(12);
    popFont.setBold(true);
    painter.setFont(popFont);
    QString popText = QString("Pop: %1/%2").arg(boardHeroCount).arg(m_populationCap);
    painter.drawText(BOARD_OFFSET_X + BOARD_PIXEL_SIZE + 35, infoY, popText);

    // ── 阶段标题 ──
    QFont uiFont;
    uiFont.setPixelSize(14);
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
        int btnW = 150, btnH = 36;
        int btnX = textX, btnY = BOARD_OFFSET_Y + 55;
        m_startButtonRect = QRect(btnX, btnY, btnW, btnH);

        bool hasBoardHero = anyHeroOnPlayerHalf();

        QColor btnC = hasBoardHero ? QColor(55, 150, 55) : QColor(75, 75, 75);
        painter.setBrush(btnC);
        painter.setPen(QPen(hasBoardHero ? QColor(90, 210, 90) : QColor(110, 110, 110), 2));
        painter.drawRoundedRect(m_startButtonRect, 8, 8);

        painter.setPen(Qt::white);
        QFont btnFont;
        btnFont.setPixelSize(13);
        btnFont.setBold(true);
        painter.setFont(btnFont);
        painter.drawText(m_startButtonRect, Qt::AlignCenter, "Start Battle");

        if (!hasBoardHero) {
            painter.setPen(QColor(200, 140, 40));
            QFont hintFont;
            hintFont.setPixelSize(10);
            painter.setFont(hintFont);
            painter.drawText(textX, btnY + btnH + 18, "Buy & drag units to the board");
        }
    }

    // ── 图例 ──
    int legendY = m_phase == GamePhase::Preparation
        ? m_startButtonRect.bottom() + 55 : BOARD_OFFSET_Y + 75;
    QFont legFont;
    legFont.setPixelSize(10);
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
    listTitleFont.setPixelSize(11);
    listTitleFont.setBold(true);
    painter.setFont(listTitleFont);
    painter.drawText(textX, unitListY, "Alive Units:");

    unitListY += 22;
    QFont listFont;
    listFont.setPixelSize(9);
    painter.setFont(listFont);
    for (auto& u : m_units) {
        if (u->isDisappeared() || u->isDead()) continue;
        bool isH = isHeroSide(u.get());
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
        failFont.setPixelSize(30);
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
    helpFont.setPixelSize(10);
    painter.setFont(helpFont);
    if (m_phase == GamePhase::Preparation) {
        painter.drawText(textX, helpY, "Click recruit slots to buy (left)");
        painter.drawText(textX, helpY + 16, "Drag equip onto heroes to equip");
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

int Synera::countBoardHeroes() const
{
    int count = 0;
    for (int y = 0; y < Board::SIZE; ++y)
        for (int x = 0; x < Board::SIZE; ++x) {
            Unit* u = m_board.getUnitAt(x, y);
            if (u && !u->isDead() && !u->isDisappeared()
                && isHeroSide(u)
                && !u->isClone())
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
            if (anyHeroOnPlayerHalf())
                startBattle();
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
                int emptySlot = findEmptyRecycleSlot();
                if (emptySlot >= 0) {
                    m_gold -= cost;
                    Unit* u = createUnitFromPool(m_recruitSlots[recruitIdx].type, true);
                    m_recycleSlots[emptySlot] = u;
                    m_recruitSlots[recruitIdx].empty = true;
                    checkAutoStarUp();
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

        // 装备合成树按钮
        if (m_synthTreeButtonRect.contains(pos)) {
            showEquipSynthWindow();
            return;
        }

        // 自定义难度按钮
        if (m_customButtonRect.contains(pos)) {
            showCustomBattleWindow();
            return;
        }

        // 装备区 / 英雄装备槽 — 开始拖拽装备。
        // 命中判定只在 processWeaponDragStart 内做一遍，
        // 拖起来则不再进入单位拖拽
        if (!m_draggedUnit && !m_draggedWeapon) {
            processWeaponDragStart(pos);
            if (m_draggedWeapon) return;
        }

        processDragStart(pos);
    }
}

void Synera::mouseMoveEvent(QMouseEvent *event)
{
    if (m_draggedUnit || m_draggedWeapon) {
        m_dragCurrentPos = event->pos();
        update();
    }
    QMainWindow::mouseMoveEvent(event);
}

void Synera::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;
    if (m_draggedWeapon) processWeaponDrop(event->pos());
    if (m_draggedUnit) processDrop(event->pos());
    QMainWindow::mouseReleaseEvent(event);
}

// ═══════════════════════════════════════════════════════════════
// 装备拖拽
// ═══════════════════════════════════════════════════════════════

void Synera::processWeaponDragStart(const QPoint& mousePos)
{
    // 1) 装备掉落区
    int dropIdx = findEquipDropAt(mousePos);
    if (dropIdx >= 0 && dropIdx < (int)m_equipDrops.size() && m_equipDrops[dropIdx]) {
        m_draggedWeapon = m_equipDrops[dropIdx];
        m_equipDrops[dropIdx] = nullptr;
        m_dragWeaponFromDropIdx = dropIdx;
        m_dragWeaponFromUnit = nullptr;
        m_dragCurrentPos = mousePos;
        // 清理空槽
        m_equipDrops.erase(
            std::remove(m_equipDrops.begin(), m_equipDrops.end(), nullptr),
            m_equipDrops.end());
        return;
    }

    // 2) 棋盘英雄的装备槽
    EquipType boardSlot;
    Unit* boardHero = findBoardEquipSlotAt(mousePos, boardSlot);
    if (boardHero && boardHero->getEquip(boardSlot)) {
        m_draggedWeapon = boardHero->getEquip(boardSlot);
        boardHero->unequip(boardSlot);
        m_dragWeaponFromDropIdx = -1;
        m_dragWeaponFromUnit = boardHero;
        m_dragWeaponFromSlot = boardSlot;
        m_dragCurrentPos = mousePos;
        return;
    }

    // 3) 回收槽英雄的装备槽
    EquipType recycleSlot;
    Unit* recycleHero = findRecycleEquipSlotAt(mousePos, recycleSlot);
    if (recycleHero && recycleHero->getEquip(recycleSlot)) {
        m_draggedWeapon = recycleHero->getEquip(recycleSlot);
        recycleHero->unequip(recycleSlot);
        m_dragWeaponFromDropIdx = -1;
        m_dragWeaponFromUnit = recycleHero;
        m_dragWeaponFromSlot = recycleSlot;
        m_dragCurrentPos = mousePos;
        return;
    }
}

void Synera::processWeaponDrop(const QPoint& mousePos)
{
    if (!m_draggedWeapon) return;

    // 放到英雄身上：先尝试合成，失败则装备；两者都不行返回 false（返还来源）
    auto tryPlaceOnHero = [&](Unit* u) -> bool {
        if (trySynthesize(u, m_draggedWeapon)) {
            m_draggedWeapon = nullptr;
            update();
            return true;
        }
        if (u->equip(m_draggedWeapon)) {
            m_draggedWeapon = nullptr;
            m_dragWeaponFromDropIdx = -1;
            m_dragWeaponFromUnit = nullptr;
            update();
            return true;
        }
        return false; // 槽位满或类型冲突
    };

    // 1) 放到棋盘英雄上
    for (int y = 0; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            Unit* u = m_board.getUnitAt(x, y);
            if (!u || u->isDead() || u->isDisappeared()) continue;
            if (!isHeroSide(u)) continue;
            if (cellRect(x, y).contains(mousePos)) {
                if (tryPlaceOnHero(u))
                    return;
                break;
            }
        }
    }

    // 2) 放到回收槽英雄上
    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 8; ++col) {
            int idx = row * 8 + col;
            Unit* u = m_recycleSlots[idx];
            if (!u || u->isDead() || u->isDisappeared()) continue;
            if (!isHeroSide(u)) continue;
            if (recycleSlotRect(row, col).contains(mousePos)) {
                if (tryPlaceOnHero(u))
                    return;
                break;
            }
        }
    }

    // 3) 放到装备掉落区 — 先检查是否与目标槽已有装备合成
    int dropTargetIdx = findEquipDropAt(mousePos);
    if (dropTargetIdx >= 0 && dropTargetIdx < (int)m_equipDrops.size() && m_equipDrops[dropTargetIdx]) {
        // 有目标装备：尝试合成
        Weapon* targetWeapon = m_equipDrops[dropTargetIdx];
        std::string resultName = getSynthesisResultName(m_draggedWeapon->getName(), targetWeapon->getName());
        if (!resultName.empty() && m_gold >= 300) {
            m_gold -= 300;
            // 移除目标槽的装备
            m_equipDrops[dropTargetIdx] = nullptr;
            // 创建高级装备放入掉落区
            Weapon* advanced = createWeaponByName(resultName);
            if (advanced)
                m_equipDrops[dropTargetIdx] = advanced;
            // 清理空槽
            m_equipDrops.erase(
                std::remove(m_equipDrops.begin(), m_equipDrops.end(), nullptr),
                m_equipDrops.end());
            m_draggedWeapon = nullptr;
            m_dragWeaponFromDropIdx = -1;
            m_dragWeaponFromUnit = nullptr;
            update();
            return;
        }
        // 无法合成，继续往下走（放回或归还）
    }

    // 4) 放回装备掉落区（空槽）
    if ((int)m_equipDrops.size() < MAX_EQUIP_DROPS) {
        m_equipDrops.push_back(m_draggedWeapon);
        m_draggedWeapon = nullptr;
        m_dragWeaponFromDropIdx = -1;
        m_dragWeaponFromUnit = nullptr;
        update();
        return;
    }

    // 5) 归还来源
    if (m_dragWeaponFromUnit) {
        m_dragWeaponFromUnit->equip(m_draggedWeapon);
    } else if (m_dragWeaponFromDropIdx >= 0) {
        if (m_dragWeaponFromDropIdx < (int)m_equipDrops.size())
            m_equipDrops[m_dragWeaponFromDropIdx] = m_draggedWeapon;
        else
            m_equipDrops.push_back(m_draggedWeapon);
    }
    m_draggedWeapon = nullptr;
    m_dragWeaponFromDropIdx = -1;
    m_dragWeaponFromUnit = nullptr;
    update();
}

// ═══════════════════════════════════════════════════════════════
// 拖拽
// ═══════════════════════════════════════════════════════════════

void Synera::processDragStart(const QPoint& mousePos)
{
    if (m_draggedWeapon) return; // 正在拖拽装备时不启动单位拖拽

    // 1) 回收槽
    int recycleIdx = findRecycleSlotAt(mousePos);
    if (recycleIdx >= 0 && m_recycleSlots[recycleIdx] != nullptr) {
        m_draggedUnit = m_recycleSlots[recycleIdx];
        m_recycleSlots[recycleIdx] = nullptr;
        m_dragFromRecycleIndex = recycleIdx;
        m_dragCurrentPos = mousePos;
        return;
    }

    // 2) 棋盘上的英雄
    Unit* clicked = findUnitAtPixel(mousePos);
    if (clicked && isHeroSide(clicked) && !clicked->isDisappeared()) {
        m_draggedUnit = clicked;
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
            m_dragFromRecycleIndex = -1;
            update();
            return;
        }
    }

    // ── 放回回收槽 ──
    int recycleIdx = findRecycleSlotAt(mousePos);
    if (recycleIdx >= 0) {
        if (!m_recycleSlots[recycleIdx]) {
            m_recycleSlots[recycleIdx] = m_draggedUnit;
            m_draggedUnit = nullptr;
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
            if (overlap <= unitArea / 2) continue;
            if (m_board.isOccupied(x, y)) continue;
            if (overlap > bestOverlap) {
                bestOverlap = overlap;
                bestGx = x; bestGy = y;
            }
        }
    }

    if (bestGx >= 0) {
        // 检查人口上限（仅当拖拽来源不是棋盘时）
        if (m_dragFromRecycleIndex >= 0) {
            if (countBoardHeroes() >= m_populationCap) {
                // 人口已满，返还回收槽
                m_recycleSlots[m_dragFromRecycleIndex] = m_draggedUnit;
                m_draggedUnit = nullptr;
                m_dragFromRecycleIndex = -1;
                update();
                return;
            }
        }
        m_board.placeUnit(m_draggedUnit, bestGx, bestGy);
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
    m_dragFromRecycleIndex = -1;
    update();
}

// ═══════════════════════════════════════════════════════════════
// 自动战斗 — 每帧调用
// ═══════════════════════════════════════════════════════════════

// 技能结算样板：快照全场 HP → 释放技能 → 记录伤害/治疗数字 → 敌方死亡发金币并掉装备
void Synera::castAndSettle(Unit* caster, void (Unit::*skill)(Board&, std::vector<Unit*>&),
                           std::vector<Unit*>& alive)
{
    std::map<Unit*, int> hpBefore;
    for (Unit* au : alive)
        if (!au->isDead() && !au->isDisappeared())
            hpBefore[au] = au->getHp();

    std::vector<Unit*> enemiesBefore;
    for (Unit* eu : alive)
        if (!eu->isDead() && !eu->isDisappeared() && isEnemySide(eu))
            enemiesBefore.push_back(eu);

    (caster->*skill)(m_board, alive);

    for (auto& kv : hpBefore) {
        if (kv.first->isDead() || kv.first->isDisappeared()) continue;
        if (kv.first->hasReviveTriggered()) continue; // 复活触发，跳过死亡判定
        int diff = kv.first->getHp() - kv.second;
        if (diff != 0)
            m_pendingDamageEvents[kv.first].push_back(diff);
    }
    for (Unit* eu : enemiesBefore) {
        if (eu->isDead() && !eu->hasReviveTriggered()) {
            m_pendingGold += enemyGoldValue(eu);
            tryEquipDrop();
        }
    }
}

// 吟咏魔典羁绊：法师施放者释放技能点时共享给其他己技巧师
void Synera::shareManaToMages(Unit* caster, const std::vector<Unit*>& alive) const
{
    if (!(m_bondActive[1] && caster->getType() == UnitType::Mage
          && isHeroSide(caster) && !caster->isClone()))
        return;
    for (Unit* mu : alive)
        if (mu != caster && mu->getType() == UnitType::Mage && isHeroSide(mu) && !mu->isClone())
            mu->gainMana();
}

void Synera::processBurningTick(std::vector<Unit*>& alive)
{
    ++m_burnTickCount;
    for (Unit* u : alive) {
        if (u->isBurning()) {
            u->tickBurning();
            if (u->isDead() && !u->hasReviveTriggered()) {
                if (isEnemySide(u)) {
                    m_pendingGold += enemyGoldValue(u);
                    tryEquipDrop();
                }
                m_board.removeUnit(u->getPosition().x, u->getPosition().y);
            }
        }
    }
}

void Synera::processAssassinSkills(std::vector<Unit*>& alive)
{
    std::vector<Unit*> skillAssassins;
    for (Unit* u : alive) {
        if (u->isDead() || u->isDisappeared()) continue;
        if (u->getType() != UnitType::Assassin) continue;
        if (u->isClone()) continue;
        if (u->getMana() < u->getMaxMana()) continue;
        skillAssassins.push_back(u);
    }
    if (skillAssassins.empty()) return;

    std::set<Unit*> deadSet;

    for (Unit* assassin : skillAssassins) {
        if (deadSet.count(assassin)) continue;
        if (assassin->isDead() || assassin->isDisappeared()) continue;
        if (assassin->getMana() < assassin->getMaxMana()) continue;

        bool isHero = isHeroSide(assassin);
        Position ap = assassin->getPosition();

        // 索敌：最近敌方，距离 ≤2
        Unit* target = nullptr;
        int bestDist = 999;
        for (Unit* eu : alive) {
            if (eu == assassin || eu->isDead() || eu->isDisappeared()) continue;
            if (deadSet.count(eu)) continue;
            bool euIsHero = isHeroSide(eu);
            if (euIsHero == isHero) continue;
            int d = manhattanDist(ap, eu->getPosition());
            if (d <= 2 && d < bestDist) { bestDist = d; target = eu; }
        }
        if (!target) continue;

        // 互杀检测：两人都是满技能刺客且互为最近目标
        bool mutualKill = false;
        if (target->getType() == UnitType::Assassin && target->getMana() >= target->getMaxMana()) {
            bool tIsHero = isHeroSide(target);
            Unit* tTarget = nullptr;
            int tBestDist = 999;
            Position tp = target->getPosition();
            for (Unit* eu : alive) {
                if (eu == target || eu->isDead() || eu->isDisappeared()) continue;
                if (deadSet.count(eu)) continue;
                bool euIsHero = isHeroSide(eu);
                if (euIsHero == tIsHero) continue;
                int d = manhattanDist(tp, eu->getPosition());
                if (d <= 2 && d < tBestDist) { tBestDist = d; tTarget = eu; }
            }
            if (tTarget == assassin) {
                mutualKill = true;
            }
        }

        if (mutualKill) {
            deadSet.insert(assassin);
            deadSet.insert(target);
            bool aEnemy = isEnemySide(assassin);
            bool tEnemy = isEnemySide(target);
            m_pendingDamageEvents[assassin].push_back(-assassin->getHp());
            m_pendingDamageEvents[target].push_back(-target->getHp());
            assassin->takeDamage(assassin->getHp());
            target->takeDamage(target->getHp());
            assassin->resetMana();
            target->resetMana();
            if (!assassin->hasReviveTriggered()) {
                if (aEnemy) { m_pendingGold += enemyGoldValue(assassin); tryEquipDrop(); }
                m_board.removeUnit(ap.x, ap.y);
            }
            if (!target->hasReviveTriggered()) {
                if (tEnemy) { m_pendingGold += enemyGoldValue(target); tryEquipDrop(); }
                m_board.removeUnit(target->getPosition().x, target->getPosition().y);
            }
            continue;
        }

        // 正常释放技能（结算样板见 castAndSettle）
        Position posBefore = assassin->getPosition();
        castAndSettle(assassin, &Unit::useSkill, alive);

        // 仅当确实瞬移了（位置改变）才重置法力值，否则保留技能点
        if (!(assassin->getPosition() == posBefore)) {
            assassin->resetMana();

            // 瞬移残影特效：从起点到终点留下高透明度幻影轨迹
            m_ghostEffects.push_back({
                posBefore.x, posBefore.y,
                assassin->getPosition().x, assassin->getPosition().y,
                static_cast<int>(assassin->getType()),
                isHero,
                m_frameCounter,
                GHOST_EFFECT_FRAMES
            });
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
    m_projectileEffects.erase(
        std::remove_if(m_projectileEffects.begin(), m_projectileEffects.end(),
            [this](const ProjectileEffect& e) { return m_frameCounter >= e.startFrame + e.duration; }),
        m_projectileEffects.end());
    m_healEffects.erase(
        std::remove_if(m_healEffects.begin(), m_healEffects.end(),
            [this](const HealEffect& e) { return m_frameCounter >= e.startFrame + e.duration; }),
        m_healEffects.end());
    m_ghostEffects.erase(
        std::remove_if(m_ghostEffects.begin(), m_ghostEffects.end(),
            [this](const GhostEffect& e) { return m_frameCounter >= e.startFrame + e.duration; }),
        m_ghostEffects.end());

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

    // 羁绊判定（每帧最开始时）
    checkAndApplyBonds(alive);

    // 燃烧伤害（每 BURNING_INTERVAL 帧）
    bool burningFrame = (m_frameCounter % BURNING_INTERVAL == 0);
    if (burningFrame)
        processBurningTick(alive);

    // 燃烧结算后立刻处理刺客技能
    if (burningFrame)
        processAssassinSkills(alive);

    // 燃烧Tick超过500次 → 超时判胜（防止治疗系角色的死循环）
    if (m_burnTickCount > 500) {
        endLevel(true);
        return;
    }

    for (Unit* u : alive) {
        if (u->isDead() || u->isDisappeared()) continue;
        Position pos = u->getPosition();

        u->incrementTimers();

        // Boss 进阶技能：第二法力值满自动释放（米字路径攻击）
        if (u->getMaxMana2() > 0 && u->getMana2() >= u->getMaxMana2()) {
            castAndSettle(u, &Unit::useSkill2, alive);
            u->resetMana2();
        }

        // ── 法力值满 → 释放技能（刺客由 processAssassinSkills 统一处理）──
        if (u->getMana() >= u->getMaxMana() && u->getType() != UnitType::Assassin) {
            bool hasValidTarget = false;
            bool isSupport = u->canHeal();

            if (isSupport) {
                bool isHero = isHeroSide(u);
                for (Unit* au : alive) {
                    if (au == u || au->isDead() || au->isDisappeared()) continue;
                    bool auIsHero = isHeroSide(au);
                    if (auIsHero != isHero) continue;
                    if (au->getHp() < au->getMaxHp()) { hasValidTarget = true; break; }
                }
            } else if (u->getType() == UnitType::Mage) {
                // 法师技能需周围5×5有敌方
                bool isHero = isHeroSide(u);
                int range = 2;
                for (Unit* eu : alive) {
                    if (eu == u || eu->isDead() || eu->isDisappeared()) continue;
                    bool euIsHero = isHeroSide(eu);
                    if (euIsHero == isHero) continue;
                    int dx = std::abs(u->getPosition().x - eu->getPosition().x);
                    int dy = std::abs(u->getPosition().y - eu->getPosition().y);
                    if (dx <= range && dy <= range) { hasValidTarget = true; break; }
                }
            } else {
                bool isHero = isHeroSide(u);
                for (Unit* eu : alive) {
                    if (eu == u || eu->isDead() || eu->isDisappeared()) continue;
                    bool euIsHero = isHeroSide(eu);
                    if (euIsHero != isHero) { hasValidTarget = true; break; }
                }
            }

            if (!hasValidTarget) {
                // 无有效目标，保留技能点，继续移动/攻击
            } else {
                // 技能视觉前摇数据：战士技能=重斩（攻击者→最近敌方）
                Unit* skillSlashTarget = nullptr;
                if (u->getType() == UnitType::Warrior)
                    skillSlashTarget = findNearestEnemyFor(u);

                // 辅助技能治疗特效判定用的 HP 快照
                std::map<Unit*, int> hpBefore;
                if (isSupport) {
                    for (Unit* au : alive) {
                        if (!au->isDead() && !au->isDisappeared())
                            hpBefore[au] = au->getHp();
                    }
                }

                castAndSettle(u, &Unit::useSkill, alive);
                u->resetMana();
                u->resetMoveTimer();
                u->resetAttackTimer();
                u->gainMana2(); // Boss 进阶技能充能（其他单位 maxMana2=0，空操作）

                if (u->getType() == UnitType::Warrior && skillSlashTarget
                    && !skillSlashTarget->isDisappeared()) {
                    m_slashEffects.push_back({
                        pos.x, pos.y,
                        skillSlashTarget->getPosition().x,
                        skillSlashTarget->getPosition().y,
                        1, m_frameCounter, SKILL_SLASH_FRAMES
                    });
                }

                // 辅助技能治疗目标 → 大 "+" 粒子群
                for (auto& kv : hpBefore) {
                    if (kv.first->isDead() || kv.first->isDisappeared()) continue;
                    int diff = kv.first->getHp() - kv.second;
                    if (diff > 0)
                        m_healEffects.push_back({
                            kv.first->getPosition().x,
                            kv.first->getPosition().y,
                            true,
                            m_frameCounter,
                            HEAL_EFFECT_FRAMES
                        });
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

                // 治疗特效：被治疗者身上冒绿色 "+"
                m_healEffects.push_back({
                    healTarget->getPosition().x,
                    healTarget->getPosition().y,
                    false,
                    m_frameCounter,
                    HEAL_EFFECT_FRAMES
                });

                m_pendingDamageEvents[healTarget].push_back(healed);
                u->gainMana();
                shareManaToMages(u, alive);
                u->resetAttackTimer();
            } else if (!inRange) {
                u->resetAttackTimer();
            }

            // 移动：仅当目标不在攻击/治疗范围内才靠近
            if (u->getMoveTimer() >= u->getMoveSpeed()) {
                if (!inRange) {
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

                    // 命中特效：战士=斩击，刺客=快速斩击，法师=火球飞行弹道
                    Position tp = target->getPosition();
                    if (u->getType() == UnitType::Warrior) {
                        m_slashEffects.push_back({
                            pos.x, pos.y, tp.x, tp.y,
                            0, m_frameCounter, SLASH_EFFECT_FRAMES
                        });
                    } else if (u->getType() == UnitType::Assassin) {
                        m_slashEffects.push_back({
                            pos.x, pos.y, tp.x, tp.y,
                            2, m_frameCounter, ASSASSIN_SLASH_FRAMES
                        });
                    } else if (u->getType() == UnitType::Mage) {
                        m_projectileEffects.push_back({
                            pos.x, pos.y, tp.x, tp.y,
                            m_frameCounter,
                            8 + 3 * manhattanDist(pos, tp)   // 距离越远飞得越久
                        });
                    }

                    u->gainMana();
                    u->gainMana2(); // Boss 进阶技能充能（其他单位空操作）
                    shareManaToMages(u, alive);
                    m_pendingDamageEvents[target].push_back(-dealt);

                    if (target->isDead() && !target->hasReviveTriggered()) {
                        bool isEnemy = isEnemySide(target);
                        if (isEnemy) { m_pendingGold += enemyGoldValue(target); tryEquipDrop(); }
                        m_board.removeUnit(target->getPosition().x, target->getPosition().y);
                    }
                    u->resetAttackTimer();
                } else if (!inRange) {
                    u->resetAttackTimer();
                }

                // 移动（独立计时器）— 仅当目标不在攻击范围内
                if (u->getMoveTimer() >= u->getMoveSpeed()) {
                    if (!inRange) {
                        Position next = moveStepToward(pos, target->getPosition());
                        if (!(next == pos)) moves.push_back({u, next});
                    }
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

    // 处理复活石触发：将复活单位移到我方半场
    for (Unit* u : alive) {
        if (u->hasReviveTriggered()) {
            // 从我方半场找空位
            bool placed = false;
            for (int y = Board::SIZE - 1; y >= Board::SIZE / 2 && !placed; --y) {
                for (int x = 0; x < Board::SIZE && !placed; ++x) {
                    if (!m_board.isOccupied(x, y)) {
                        Position oldPos = u->getPosition();
                        m_board.removeUnit(oldPos.x, oldPos.y);
                        m_board.placeUnit(u, x, y);
                        placed = true;
                    }
                }
            }
            // 无论是否成功放置都必须清除触发标记，否则残留标记会导致
            // 该单位后续死亡时所有清理代码均被跳过，最终战斗无法正确结算
            u->clearReviveTriggered();
        }
    }

    // 清理场上残留的死亡单位（部分死亡路径如Boss技能、燃烧结算
    // 及复活石触发后再次被击杀的单位可能未被及时移除）
    for (int y = 0; y < Board::SIZE; ++y)
        for (int x = 0; x < Board::SIZE; ++x) {
            Unit* u = m_board.getUnitAt(x, y);
            if (u && u->isDead())
                m_board.removeUnit(x, y);
        }

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
    Q_UNUSED(alive); // 接口保留 alive 以备后续按存活状态过滤
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
                duration
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
    bool isHero = isHeroSide(unit);
    Unit* best = nullptr;
    int bestDistSq = 999999;
    int bestXDist = 999;

    for (int y = 0; y < Board::SIZE; ++y)
        for (int x = 0; x < Board::SIZE; ++x) {
            Unit* u = m_board.getUnitAt(x, y);
            if (!u || u == unit || u->isDead() || u->isDisappeared()) continue;
            bool uIsHero = isHeroSide(u);
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
    bool isHero = isHeroSide(support);
    Unit* best = nullptr;
    int bestDistSq = 999999;
    UnitType bestType = UnitType::Support;
    int bestXDist = 999;

    for (int y = 0; y < Board::SIZE; ++y)
        for (int x = 0; x < Board::SIZE; ++x) {
            Unit* u = m_board.getUnitAt(x, y);
            if (!u || u == support || u->isDead() || u->isDisappeared()) continue;
            if (u->getHp() >= u->getMaxHp()) continue;
            bool uIsHero = isHeroSide(u);
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
    bool isHero = isHeroSide(unit);
    Unit* best = nullptr;
    int bestDistSq = 999999;
    int bestXDist = 999;

    for (int y = 0; y < Board::SIZE; ++y)
        for (int x = 0; x < Board::SIZE; ++x) {
            Unit* u = m_board.getUnitAt(x, y);
            if (!u || u == unit || u->isDead() || u->isDisappeared()) continue;
            bool uIsHero = isHeroSide(u);
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

    // 优先前进（纵向），其次左右，最后后退 — 避免双方错位一格时镜像循环卡死
    if (dy != 0) {
        Position next(from.x, from.y + dy);
        if (m_board.isValidPosition(next.x, next.y) && !m_board.isOccupied(next.x, next.y))
            return next;
    }
    if (dx != 0) {
        Position next(from.x + dx, from.y);
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
// 羁绊系统
// ═══════════════════════════════════════════════════════════════

void Synera::applyBondEffect(int idx, std::vector<Unit*>& warriors, std::vector<Unit*>& mages,
                              std::vector<Unit*>& supports, std::vector<Unit*>& assassins, std::vector<Unit*>& alive)
{
    Q_UNUSED(mages); // 法师羁绊（吟咏魔典）在回蓝处实时处理，此处无需遍历
    switch (idx) {
    case 0: // 战斗不息
        for (Unit* w : warriors) w->applyBondHpMult(2.0);
        break;
    case 1: // 吟咏魔典 — 共享技能点（在 gainMana 时实时分发）
        break;
    case 2: // 生生不息
        for (Unit* s : supports) {
            s->applyBondHealMult(2.0);
            s->applyBondRangeBonus(1);
        }
        break;
    case 3: // 暗夜幻影
        spawnAssassinClones(assassins, alive);
        break;
    case 4: // 全军出击
        for (Unit* u : alive) {
            if (!isHeroSide(u)) continue;
            u->applyBondManaMod(-20);
            u->applyBondAtkBonus(20);
            u->applyBondHpMult(1.5);
        }
        break;
    }
}

void Synera::revertBondEffect(int idx, std::vector<Unit*>& alive)
{
    switch (idx) {
    case 0: // 战斗不息
        for (Unit* u : alive) {
            if (!isHeroSide(u)) continue;
            if (u->getType() == UnitType::Warrior && !u->isClone())
                u->revertBondHpMult(2.0);
        }
        break;
    case 1: // 吟咏魔典 — 无持久效果
        break;
    case 2: // 生生不息
        for (Unit* u : alive) {
            if (!isHeroSide(u)) continue;
            if (u->getType() == UnitType::Support && !u->isClone()) {
                u->revertBondHealMult(2.0);
                u->revertBondRangeBonus(1);
            }
        }
        break;
    case 3: // 暗夜幻影
        removeAssassinClones();
        break;
    case 4: // 全军出击
        for (Unit* u : alive) {
            if (!isHeroSide(u)) continue;
            u->revertBondManaMod(-20);
            u->revertBondAtkBonus(20);
            u->revertBondHpMult(1.5);
        }
        break;
    }
}

// 羁绊激活阈值（唯一来源，实时判定与准备阶段预览共用）
void Synera::bondStatesFromCounts(int warriorCount, int mageCount,
                                  int supportCount, int assassinCount, bool outActive[5])
{
    outActive[0] = (warriorCount >= 3);
    outActive[1] = (mageCount >= 2);
    outActive[2] = (supportCount >= 2);
    outActive[3] = (assassinCount >= 2);
    outActive[4] = (warriorCount > 0 && mageCount > 0 && supportCount > 0 && assassinCount > 0);
}

void Synera::checkAndApplyBonds(std::vector<Unit*>& alive)
{
    // 统计棋盘上玩家方英雄类型
    std::vector<Unit*> warriors, mages, supports, assassins;
    for (Unit* u : alive) {
        if (!isHeroSide(u)) continue;
        switch (u->getType()) {
            case UnitType::Warrior: warriors.push_back(u); break;
            case UnitType::Mage: mages.push_back(u); break;
            case UnitType::Support: supports.push_back(u); break;
            case UnitType::Assassin: assassins.push_back(u); break;
            default: break;
        }
    }

    bool newBond[5];
    bondStatesFromCounts((int)warriors.size(), (int)mages.size(),
                         (int)supports.size(), (int)assassins.size(), newBond);

    for (int i = 0; i < 5; ++i) {
        if (newBond[i] && !m_bondActive[i]) {
            // 羁绊激活
            applyBondEffect(i, warriors, mages, supports, assassins, alive);
        } else if (!newBond[i] && m_bondActive[i]) {
            // 羁绊失效
            revertBondEffect(i, alive);
        }
        m_bondActive[i] = newBond[i];
    }
}

void Synera::previewBonds()
{
    // 准备阶段预览：只判定不生效
    std::vector<Unit*> heroes = collectSurvivingHeroes();
    int warriorCount = 0, mageCount = 0, supportCount = 0, assassinCount = 0;
    for (Unit* u : heroes) {
        switch (u->getType()) {
            case UnitType::Warrior: ++warriorCount; break;
            case UnitType::Mage: ++mageCount; break;
            case UnitType::Support: ++supportCount; break;
            case UnitType::Assassin: ++assassinCount; break;
            default: break;
        }
    }
    bondStatesFromCounts(warriorCount, mageCount, supportCount, assassinCount, m_bondActive);
}

void Synera::spawnAssassinClones(const std::vector<Unit*>& assassins, std::vector<Unit*>& alive)
{
    // 在 (cx,cy) 生成分身（星-1、0 法力、继承已激活的全体羁绊）
    auto spawnCloneAt = [&](Unit* src, int cx, int cy) {
        Unit* clone = createUnitFromPool(UnitType::Assassin, true,
                                         std::max(0, src->getStarLevel() - 1));
        clone->setClone(true);
        clone->setMana(0); // 分身从0法力值开始
        if (m_bondActive[4]) {
            clone->applyBondManaMod(-20);
            clone->applyBondAtkBonus(20);
            clone->applyBondHpMult(1.5);
        }
        m_board.placeUnit(clone, cx, cy);
        alive.push_back(clone);
    };

    for (Unit* src : assassins) {
        bool placed = false;

        // 先在我方半场随机找空位
        for (int attempt = 0; attempt < 50 && !placed; ++attempt) {
            int cx = std::rand() % Board::SIZE;
            int cy = Board::SIZE / 2 + std::rand() % (Board::SIZE / 2);
            if (!m_board.isOccupied(cx, cy)) {
                spawnCloneAt(src, cx, cy);
                placed = true;
            }
        }

        // 随机失败 → 顺序扫描兜底
        if (!placed) {
            for (int y = Board::SIZE / 2; y < Board::SIZE && !placed; ++y) {
                for (int x = 0; x < Board::SIZE; ++x) {
                    if (!m_board.isOccupied(x, y)) {
                        spawnCloneAt(src, x, y);
                        placed = true;
                        break;
                    }
                }
            }
        }
    }
}

void Synera::removeAssassinClones()
{
    std::vector<Unit*> clonesToRemove;
    for (auto& u : m_units) {
        if (u->isClone() && u->getType() == UnitType::Assassin && !u->isDead() && !u->isDisappeared()) {
            clonesToRemove.push_back(u.get());
        }
    }
    for (Unit* c : clonesToRemove) {
        m_board.removeUnit(c->getPosition().x, c->getPosition().y);
        c->setDisappeared(true);
    }
}

// ═══════════════════════════════════════════════════════════════
// 装备合成系统
// ═══════════════════════════════════════════════════════════════

// 合成配方查询：返回合成结果名称，若无法合成则返回空字符串
static std::string getSynthesisResultName(const std::string& a, const std::string& b)
{
    // 按字母序排序两个名称以处理任意顺序
    auto orderPair = [](const std::string& x, const std::string& y) -> std::pair<std::string, std::string> {
        if (x < y) return {x, y};
        return {y, x};
    };
    auto p = orderPair(a, b);

    if (p.first == "Blue Crystal" && p.second == "Iron Sword")
        return "Rune Greatsword";
    if (p.first == "Iron Sword" && p.second == "Speed Gloves")
        return "Swift Blade";
    if (p.first == "Chain Mail" && p.second == "Iron Sword")
        return "Thorns Armor";
    if (p.first == "Chain Mail" && p.second == "Chain Mail")
        return "Vitality Armor";
    if (p.first == "Blue Crystal" && p.second == "Speed Gloves")
        return "Gale Gloves";
    if (p.first == "Blue Crystal" && p.second == "Chain Mail")
        return "Revive Stone";
    if (p.first == "Iron Sword" && p.second == "Warhorse")
        return "Sniper Crossbow";
    return "";
}

Weapon* Synera::createWeaponByName(const std::string& name)
{
    // 名称→类的注册表集中在 Weapon::create（weapon.cpp），这里只管所有权
    Weapon* wp = Weapon::create(name);
    if (wp)
        m_weapons.emplace_back(wp);
    return wp;
}

bool Synera::trySynthesize(Unit* hero, Weapon* draggedWeapon)
{
    if (!hero || !draggedWeapon) return false;
    if (m_gold < 300) return false;

    // 找到英雄身上能与 draggedWeapon 合成的装备
    Weapon* heroWeapon = nullptr;
    EquipType heroSlot = EquipType::Attack;
    for (int ei = 0; ei < static_cast<int>(EquipType::COUNT); ++ei) {
        EquipType et = static_cast<EquipType>(ei);
        Weapon* ew = hero->getEquip(et);
        if (!ew) continue;
        if (ew->getEquipType() == draggedWeapon->getEquipType()) continue; // 同类型不能合成
        // 检查是否是基本装备（非高级）
        std::string en = ew->getName();
        bool isBasic = (en == "Iron Sword" || en == "Chain Mail" || en == "Speed Gloves"
                     || en == "Blue Crystal" || en == "Warhorse");
        if (isBasic) { heroWeapon = ew; heroSlot = et; break; }
    }

    if (!heroWeapon) return false;

    std::string resultName = getSynthesisResultName(heroWeapon->getName(), draggedWeapon->getName());
    if (resultName.empty()) return false;

    // 扣金币
    m_gold -= 300;

    // 卸下英雄身上那个基础装备
    hero->unequip(heroSlot);

    // 拖拽的装备不会被装上（被消耗），设为 nullptr
    m_draggedWeapon = nullptr;
    m_dragWeaponFromDropIdx = -1;
    m_dragWeaponFromUnit = nullptr;

    // 创建高级装备并装到英雄身上
    Weapon* advanced = createWeaponByName(resultName);
    if (advanced)
        hero->equip(advanced);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// 复活逻辑：见 Unit::takeDamage（消耗复活石并满血）与
// processCombatFrame（复活后移回我方半场），无独立函数
// ═══════════════════════════════════════════════════════════════

void Synera::renderBonds(QPainter& painter)
{
    // 准备阶段预览羁绊状态，战斗阶段由 processCombatFrame 实时判定
    if (m_phase == GamePhase::Preparation)
        previewBonds();

    if (m_popUpgradeButtonRect.isNull()) return;
    int bondStartY = m_popUpgradeButtonRect.bottom() + 10;
    int bondX = LEFT_PANEL_X;

    struct BondUIData {
        const char* name;
        const char* desc;
    };
    static const BondUIData bondData[5] = {
        {"战斗不息", "3战士:生命翻倍"},
        {"吟咏魔典", "2法师:共享技能点"},
        {"生生不息", "2辅助:治疗翻倍+范围+1"},
        {"暗夜幻影", "2刺客:生成分身(星-1)"},
        {"全军出击", "4职齐:全属性上升"},
    };

    QFont nameFont;
    nameFont.setPixelSize(8);
    nameFont.setBold(true);
    QFont descFont;
    descFont.setPixelSize(6);

    for (int i = 0; i < 5; ++i) {
        int by = bondStartY + i * 28;
        int boxSize = 8;
        QRect boxRect(bondX, by + 2, boxSize, boxSize);

        // 羁绊激活指示框
        if (m_bondActive[i]) {
            painter.setBrush(QColor(255, 180, 40));
            painter.setPen(QPen(QColor(255, 220, 80), 1));
        } else {
            painter.setBrush(QColor(40, 40, 50));
            painter.setPen(QPen(QColor(80, 80, 90), 1));
        }
        painter.drawRoundedRect(boxRect, 1, 1);

        // 羁绊名称
        painter.setFont(nameFont);
        painter.setPen(m_bondActive[i] ? QColor(255, 200, 60) : QColor(140, 140, 160));
        painter.drawText(bondX + boxSize + 4, by + 8, bondData[i].name);

        // 羁绊描述
        painter.setFont(descFont);
        painter.setPen(m_bondActive[i] ? QColor(200, 170, 80) : QColor(100, 100, 120));
        painter.drawText(bondX + boxSize + 4, by + 18, bondData[i].desc);
    }
}

// ═══════════════════════════════════════════════════════════════
// 关卡结束判定
// ═══════════════════════════════════════════════════════════════

void Synera::checkLevelEnd()
{
    
    // 直接扫描棋盘判定存活单位（棋盘是物理真实，比 m_units 更可靠，
    // 避免复活石触发后残留的死亡/消失单位在 m_units 中被误判为存活）
    bool hasHeroAny = false, hasEnemyAny = false;
    for (int y = 0; y < Board::SIZE; ++y)
        for (int x = 0; x < Board::SIZE; ++x) {
            Unit* u = m_board.getUnitAt(x, y);
            if (!u || u->isDead() || u->isDisappeared()) continue;
            if (isHeroSide(u))
                hasHeroAny = true;
            else
                hasEnemyAny = true;
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
