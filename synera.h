#ifndef SYNERA_H
#define SYNERA_H

#include <QMainWindow>
#include <QTimer>
#include <QElapsedTimer>
#include <QPixmap>
#include <vector>
#include <map>
#include <memory>
#include "ui_synera.h"
#include "board.h"
#include "unit.h"
#include "weapon.h"

enum class GamePhase { Preparation, Battle };

struct PoolSlot {
    UnitType type;
    int count;
};

struct RecruitSlot {
    UnitType type;
    int price;
    bool empty;
};

struct HitEffect {
    int cellX, cellY;
    int amount;          // negative=damage, positive=heal
    int startFrame;
    int duration;
};

struct SlashEffect {
    int fromX, fromY;    // 攻击者所在格子
    int cellX, cellY;    // 目标所在格子
    int kind;            // 0=战士普攻斩击 1=战士技能重斩 2=刺客快速斩击
    int startFrame;
    int duration;
};

struct ProjectileEffect {          // 法师火球飞行弹道
    int fromX, fromY;    // 发射者格子
    int toX, toY;        // 目标格子
    int startFrame;
    int duration;
};

struct HealEffect {                // 辅助治疗 "+" 粒子
    int cellX, cellY;
    bool isSkill;        // true=技能大"+" + 小"+"群, false=普攻中"+"
    int startFrame;
    int duration;
};

struct GhostEffect {               // 刺客瞬移残影
    int fromX, fromY;    // 瞬移起点格子
    int toX, toY;        // 瞬移终点格子
    int type;            // UnitType 枚举值
    bool isHero;
    int startFrame;
    int duration;
};

class EquipSynthWindow;
class CustomBattleWindow;

class Synera : public QMainWindow
{
    Q_OBJECT

public:
    explicit Synera(QWidget *parent = nullptr);
    ~Synera() override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void gameLoop();
    void showEquipSynthWindow();
    void showCustomBattleWindow();
    void startCustomBattle();

private:
    void initGame();
    void initLevel();
    void startBattle();
    bool placeEnemyRandom(Unit* eu);   // 在敌方半场随机放置一个敌方单位
    void endLevel(bool playerWon);
    Unit* createUnitFromPool(UnitType type, bool isHero, int starLevel = 0, bool isBoss = false);
    Unit* createUpgradedHero(UnitType type, int starLevel);
    bool tryStarUp(int boardX, int boardY, Unit* draggedUnit);
    void checkAutoStarUp();

    void saveGame(const QString& filePath);
    void loadGame(const QString& filePath);
    void refreshRecruitment();

    void renderBoard(QPainter& painter);
    void renderUnits(QPainter& painter);
    void renderHeroInfo(QPainter& painter);
    void renderRecruitment(QPainter& painter);
    void renderRecycleSlots(QPainter& painter);
    void renderEquipDrops(QPainter& painter);
    void renderDragGhost(QPainter& painter);
    void renderSlashEffects(QPainter& painter);
    void renderProjectiles(QPainter& painter);
    void renderHealEffects(QPainter& painter);
    void renderGhostEffects(QPainter& painter);
    void renderUI(QPainter& painter);
    void renderBonds(QPainter& painter);

    QRect cellRect(int x, int y) const;
    QRect recruitSlotRect(int index) const;
    QRect recycleSlotRect(int row, int col) const;

    void processDragStart(const QPoint& mousePos);
    void processDrop(const QPoint& mousePos);
    void processWeaponDrop(const QPoint& mousePos);
    void processWeaponDragStart(const QPoint& mousePos);

    // 装备槽点击检测
    Unit* findBoardEquipSlotAt(const QPoint& pixel, EquipType& outType) const;
    Unit* findRecycleEquipSlotAt(const QPoint& pixel, EquipType& outType) const;

    // 自动战斗（每帧调用）
    void processCombatFrame();
    void processBurningTick(std::vector<Unit*>& alive);
    void processAssassinSkills(std::vector<Unit*>& alive);
    void tryEquipDrop();
    Unit* findNearestEnemyFor(Unit* unit) const;
    Unit* findHealTarget(Unit* support) const;
    Unit* findNearestAlly(Unit* unit) const;
    Position moveStepToward(const Position& from, const Position& to) const;
    Position moveStepTowardAlly(const Position& from, const Position& to) const;
    bool canAttack(Unit* attacker, Unit* target) const;
    void checkLevelEnd();

    // 技能结算样板：快照 HP → 释放技能 → 记录伤害数字 → 敌方死亡发金币/掉装备
    void castAndSettle(Unit* caster, void (Unit::*skill)(Board&, std::vector<Unit*>&),
                       std::vector<Unit*>& alive);

    // 关卡结束：收集棋盘存活英雄 / 移入回收槽（放不下则消失）
    std::vector<Unit*> collectSurvivingHeroes() const;
    void retireHeroesToRecycle(std::vector<Unit*> heroes);

    // 吟咏魔典羁绊：法师施放者把技能点共享给其他己技巧师
    void shareManaToMages(Unit* caster, const std::vector<Unit*>& alive) const;

    // 羁绊状态计算（阈值表达式唯一来源）
    static void bondStatesFromCounts(int warriorCount, int mageCount,
                                     int supportCount, int assassinCount, bool outActive[5]);

    // 布局/命中检测辅助
    bool anyHeroOnPlayerHalf() const;
    int  findEmptyRecycleSlot() const;
    int  boardEquipBoxWidth(const Unit* u) const;    // 棋盘单位装备框统一宽度（渲染与命中共用）
    int  recycleEquipBoxWidth(const Unit* u) const;  // 回收槽单位装备框统一宽度

    // 羁绊系统
    void checkAndApplyBonds(std::vector<Unit*>& alive);
    void previewBonds();  // 准备阶段预览羁绊状态
    void spawnAssassinClones(const std::vector<Unit*>& assassins, std::vector<Unit*>& alive);
    void removeAssassinClones();
    void applyBondEffect(int idx, std::vector<Unit*>& warriors, std::vector<Unit*>& mages,
                         std::vector<Unit*>& supports, std::vector<Unit*>& assassins, std::vector<Unit*>& alive);
    void revertBondEffect(int idx, std::vector<Unit*>& alive);

    Unit* findUnitAtPixel(const QPoint& pixel) const;
    int   findRecruitSlotAt(const QPoint& pixel) const;
    int   findRecycleSlotAt(const QPoint& pixel) const;
    int   findEquipDropAt(const QPoint& pixel) const;

    int enemyGoldValue(const Unit* u) const;
    int heroCost(UnitType t) const;

    Ui::MainWindow *ui;
    QTimer *m_gameTimer;
    QElapsedTimer m_frameClock;

    Board m_board;
    std::vector<std::unique_ptr<Unit>> m_units;
    std::vector<std::unique_ptr<Weapon>> m_weapons;
    std::vector<PoolSlot> m_shop;          // 英雄信息面板（4种类型）
    std::vector<RecruitSlot> m_recruitSlots; // 招募区 5 槽

    GamePhase m_phase;
    bool m_gameOver;
    bool m_playerVictory;
    bool m_showLevelLoss;                     // 关卡失败提示
    int m_frameCounter;                     // 全局帧计数
    int m_burnTickCount;                    // processBurningTick 执行次数，超过500则超时判胜

    // 关卡
    int m_currentLevel;
    static constexpr int MAX_LEVEL = 5;

    // 玩家
    int m_playerHp;
    int m_gold;
    int m_pendingGold;                      // 本关战斗中累积金币

    // 回收槽：2行 × 8列
    std::vector<Unit*> m_recycleSlots;      // 16个

    // 拖拽（来源只需记录回收槽索引；棋盘来源无需标记）
    Unit* m_draggedUnit = nullptr;
    QPoint m_dragCurrentPos;
    int m_dragFromRecycleIndex = -1;        // recycle slot index, -1 if from board

    // 装备拖拽
    Weapon* m_draggedWeapon = nullptr;
    int m_dragWeaponFromDropIdx = -1;       // equip drop index, -1 if from hero
    Unit* m_dragWeaponFromUnit = nullptr;   // hero unequipped from, nullptr if from drop
    EquipType m_dragWeaponFromSlot = EquipType::Attack; // equip type slot on hero

    // 装备掉落
    std::vector<Weapon*> m_equipDrops;
    std::vector<QRect> m_equipDropRects;

    static constexpr int MAX_EQUIP_DROPS = 10;

    // 装备合成
    bool trySynthesize(Unit* hero, Weapon* draggedWeapon);
    Weapon* createWeaponByName(const std::string& name);

    // 按钮区域

    QRect m_startButtonRect;
    QRect m_refreshButtonRect;              // 刷新招募区按钮
    std::vector<QRect> m_recruitRects;      // 招募区 5 槽点击区域
    QRect m_popUpgradeButtonRect;           // 人口上限升级按钮
    QRect m_synthTreeButtonRect;            // 装备合成树按钮

    // 装备合成树窗口（非模态，指针管理生命周期）
    EquipSynthWindow* m_equipSynthWindow = nullptr;

    // 自定义难度窗口（非模态独立渲染层）+ 自定义战斗状态
    CustomBattleWindow* m_customBattleWindow = nullptr;
    QRect m_customButtonRect;             // 主界面“自定义难度”按钮
    bool m_customBattle = false;          // 当前战斗是否为自定义战斗

    // 人口上限
    int m_populationCap;
    int countBoardHeroes() const;

    // 羁绊状态
    bool m_bondActive[5] = {false, false, false, false, false};

    // 视觉特效
    std::vector<HitEffect> m_hitEffects;
    std::vector<SlashEffect> m_slashEffects;
    std::vector<ProjectileEffect> m_projectileEffects;
    std::vector<HealEffect> m_healEffects;
    std::vector<GhostEffect> m_ghostEffects;

    // 各职业立绘贴图（按 UnitType 索引，含 Boss；加载失败为空 → 回退色块渲染）
    QPixmap m_unitPortraits[static_cast<int>(UnitType::Boss) + 1];
    void loadUnitPortraits();
    static constexpr int SLASH_EFFECT_FRAMES = 14;   // 战士普攻斩击持续帧数
    static constexpr int SKILL_SLASH_FRAMES = 20;    // 战士技能重斩持续帧数
    static constexpr int ASSASSIN_SLASH_FRAMES = 10; // 刺客快速斩击持续帧数
    static constexpr int HEAL_EFFECT_FRAMES = 30;    // 治疗 "+" 粒子持续帧数
    static constexpr int GHOST_EFFECT_FRAMES = 18;   // 刺客瞬移残影持续帧数

    // 伤害/治疗累积显示
    std::map<Unit*, std::vector<int>> m_pendingDamageEvents;
    void flushDamageEvents(const std::vector<Unit*>& alive);
    int fastestAttackSpeed() const;

    static constexpr int CELL_SIZE = 56;
    static constexpr int BOARD_OFFSET_X = 148;
    static constexpr int BOARD_OFFSET_Y = 56;
    static constexpr int BOARD_PIXEL_SIZE = CELL_SIZE * Board::SIZE;
    static constexpr int LEFT_PANEL_X = 8;
    static constexpr int LEFT_PANEL_W = 124;
    static constexpr int INFO_PANEL_Y = 76;
    static constexpr int INFO_PANEL_H = 48;
    static constexpr int INFO_SPACING = 4;
    static constexpr int RECRUIT_START_Y = 318;
    static constexpr int RECRUIT_SLOT_H = 36;
    static constexpr int RECRUIT_SPACING = 4;
    static constexpr int RECYCLE_Y = BOARD_OFFSET_Y + BOARD_PIXEL_SIZE + 16;
    static constexpr int RECYCLE_SLOT_W = 44;
    static constexpr int RECYCLE_SLOT_H = 40;
    static constexpr int RECYCLE_SPACING = 4;
    // 回收槽区域整体宽度与起始 X（布局公式唯一来源）
    static constexpr int RECYCLE_TOTAL_W = 8 * RECYCLE_SLOT_W + 7 * RECYCLE_SPACING;
    static constexpr int RECYCLE_START_X = BOARD_OFFSET_X + (BOARD_PIXEL_SIZE - RECYCLE_TOTAL_W) / 2;
    static constexpr int BURNING_INTERVAL = 60;
};

#endif // SYNERA_H
