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

enum class GamePhase { Placement, Battle };

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
    void generateUnitPool();
    void startBattle();
    Unit* createUnitFromPool(UnitType type, bool isHero);

    void renderBoard(QPainter& painter);
    void renderUnits(QPainter& painter);
    void renderBenchPool(QPainter& painter);
    void renderDragGhost(QPainter& painter);
    void renderUI(QPainter& painter);

    QRect cellRect(int x, int y) const;
    QRect poolSlotRect(int index) const;

    void processDragStart(const QPoint& mousePos);
    void processDrop(const QPoint& mousePos);

    // 自动战斗
    void processCombatTick();
    void processBurningTick(std::vector<Unit*>& alive);
    Unit* findNearestEnemyFor(Unit* unit) const;
    Unit* findHealTarget(Unit* support) const;
    Position moveStepToward(const Position& from, const Position& to) const;
    bool canAttack(Unit* attacker, Unit* target) const;
    void checkWinCondition();

    Unit* findUnitAt(int boardX, int boardY) const;
    Unit* findUnitAtPixel(const QPoint& pixel) const;
    int   findPoolSlotAt(const QPoint& pixel) const;

    Ui::MainWindow *ui;
    QTimer *m_gameTimer;
    QElapsedTimer m_frameClock;

    Board m_board;
    std::vector<std::unique_ptr<Unit>> m_units;
    std::vector<std::unique_ptr<Weapon>> m_weapons;
    std::vector<PoolSlot> m_unitPool;

    GamePhase m_phase;
    bool m_gameOver;
    int m_combatTickCounter;

    Unit* m_draggedUnit;
    QPoint m_dragCurrentPos;
    int m_dragFromPoolIndex; // -1 = from board

    QRect m_startButtonRect;

    static constexpr int CELL_SIZE = 80;
    static constexpr int BOARD_OFFSET_X = 200;
    static constexpr int BOARD_OFFSET_Y = 80;
    static constexpr int BOARD_PIXEL_SIZE = CELL_SIZE * Board::SIZE;
    static constexpr int POOL_X = 10;
    static constexpr int POOL_Y = 80;
    static constexpr int POOL_SLOT_H = 100;
    static constexpr int POOL_WIDTH = 170;
    static constexpr int TICK_INTERVAL = 18; // ~0.3s per tick
};

#endif // SYNERA_H
