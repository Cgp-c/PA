#ifndef SYNERA_H
#define SYNERA_H

#include <QMainWindow>
#include <QTimer>
#include <QElapsedTimer>
#include <vector>
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
    Unit* createUnitFromPool(UnitType type, bool isHero, int starLevel = 0);
    Unit* createUpgradedHero(UnitType type, int starLevel);
    bool tryStarUp(int boardX, int boardY, Unit* draggedUnit);

    void saveGame(const QString& filePath);
    void loadGame(const QString& filePath);

    void renderBoard(QPainter& painter);
    void renderUnits(QPainter& painter);
    void renderShop(QPainter& painter);
    void renderRecycleSlots(QPainter& painter);
    void renderDragGhost(QPainter& painter);
    void renderUI(QPainter& painter);

    QRect cellRect(int x, int y) const;
    QRect shopBuyRect(int index) const;
    QRect recycleSlotRect(int row, int col) const;

    void processDragStart(const QPoint& mousePos);
    void processDrop(const QPoint& mousePos);

    // 自动战斗（每帧调用）
    void processCombatFrame();
    void processBurningTick(std::vector<Unit*>& alive);
    Unit* findNearestEnemyFor(Unit* unit) const;
    Unit* findHealTarget(Unit* support) const;
    Unit* findNearestAlly(Unit* unit) const;
    Position moveStepToward(const Position& from, const Position& to) const;
    Position moveStepTowardAlly(const Position& from, const Position& to) const;
    bool canAttack(Unit* attacker, Unit* target) const;
    void checkLevelEnd();

    Unit* findUnitAt(int boardX, int boardY) const;
    Unit* findUnitAtPixel(const QPoint& pixel) const;
    int   findShopSlotAt(const QPoint& pixel) const;
    int   findRecycleSlotAt(const QPoint& pixel) const;

    int enemyGoldValue(UnitType t) const;
    int heroCost(UnitType t) const;

    Ui::MainWindow *ui;
    QTimer *m_gameTimer;
    QElapsedTimer m_frameClock;

    Board m_board;
    std::vector<std::unique_ptr<Unit>> m_units;
    std::vector<std::unique_ptr<Weapon>> m_weapons;
    std::vector<PoolSlot> m_shop;          // 商店库存（可购买数量）

    GamePhase m_phase;
    bool m_gameOver;
    bool m_playerVictory;
    bool m_showLevelLoss;                     // 关卡失败提示
    int m_frameCounter;                     // 全局帧计数

    // 关卡
    int m_currentLevel;
    static constexpr int MAX_LEVEL = 3;

    // 玩家
    int m_playerHp;
    int m_gold;
    int m_pendingGold;                      // 本关战斗中累积金币

    // 回收槽：2行 × 4类型，存指向 m_units 的裸指针
    std::vector<Unit*> m_recycleSlots;      // 8个

    // 拖拽
    Unit* m_draggedUnit;
    QPoint m_dragCurrentPos;
    int m_dragFromShopIndex;                // -1=board, -2=recycle, >=0=shop index
    int m_dragFromRecycleIndex;             // recycle slot index (0-7)

    // 按钮区域
    QRect m_startButtonRect;
    std::vector<QRect> m_buyButtonRects;    // 商店购买按钮

    static constexpr int CELL_SIZE = 64;
    static constexpr int BOARD_OFFSET_X = 160;
    static constexpr int BOARD_OFFSET_Y = 64;
    static constexpr int BOARD_PIXEL_SIZE = CELL_SIZE * Board::SIZE;
    static constexpr int SHOP_X = 10;
    static constexpr int SHOP_Y = 64;
    static constexpr int SHOP_SLOT_H = 88;
    static constexpr int SHOP_PANEL_W = 62;
    static constexpr int SHOP_ICON_W = 70;
    static constexpr int RECYCLE_Y = BOARD_OFFSET_Y + BOARD_PIXEL_SIZE + 20;
    static constexpr int RECYCLE_SLOT_W = 56;
    static constexpr int RECYCLE_SLOT_H = 48;
    static constexpr int RECYCLE_SPACING = 8;
    static constexpr int BURNING_INTERVAL = 30;
};

#endif // SYNERA_H
