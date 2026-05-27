#ifndef SYNERA_H
#define SYNERA_H

#include <QMainWindow>
#include <QTimer>
#include <QElapsedTimer>
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
    bool alive;          // true=独立悬浮文本，false=随单位消失
};

struct SlashEffect {
    int cellX, cellY;
    bool isSkill;        // false=attack slash, true=skill slash
    int startFrame;
    int duration;
};

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

private:
    void initGame();
    void initLevel();
    void startBattle();
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
    void renderUI(QPainter& painter);

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

    Unit* findUnitAt(int boardX, int boardY) const;
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

    // 关卡
    int m_currentLevel;
    static constexpr int MAX_LEVEL = 5;

    // 玩家
    int m_playerHp;
    int m_gold;
    int m_pendingGold;                      // 本关战斗中累积金币

    // 回收槽：2行 × 8列
    std::vector<Unit*> m_recycleSlots;      // 16个

    // 拖拽
    Unit* m_draggedUnit;
    QPoint m_dragCurrentPos;
    int m_dragFromShopIndex;                // -1=board, -2=recycle, >=0=recruit index
    int m_dragFromRecycleIndex;             // recycle slot index (0-15)

    // 装备拖拽
    Weapon* m_draggedWeapon;
    int m_dragWeaponFromDropIdx;            // equip drop index, -1 if from hero
    Unit* m_dragWeaponFromUnit;             // hero unequipped from, nullptr if from drop
    EquipType m_dragWeaponFromSlot;         // equip type slot on hero

    // 装备掉落
    std::vector<Weapon*> m_equipDrops;
    Weapon* m_pendingEquip;
    std::vector<QRect> m_equipDropRects;
    int m_equipDropCap;
    int m_equipDropCount;

    static constexpr int MAX_EQUIP_DROPS = 8;

    // 按钮区域

    QRect m_startButtonRect;
    QRect m_refreshButtonRect;              // 刷新招募区按钮
    std::vector<QRect> m_recruitRects;      // 招募区 5 槽点击区域
    QRect m_popUpgradeButtonRect;           // 人口上限升级按钮

    // 人口上限
    int m_populationCap;
    int countBoardHeroes() const;

    // 视觉特效
    std::vector<HitEffect> m_hitEffects;
    std::vector<SlashEffect> m_slashEffects;

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
    static constexpr int BURNING_INTERVAL = 60;
};

#endif // SYNERA_H
