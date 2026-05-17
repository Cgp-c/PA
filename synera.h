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

class Synera : public QMainWindow
{
    Q_OBJECT

public:
    explicit Synera(QWidget *parent = nullptr);
    ~Synera() override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void gameLoop();

private:
    void initGame();
    void initUnits();
    void renderBoard(QPainter& painter);
    void renderUnits(QPainter& painter);
    void renderUI(QPainter& painter);
    QRect cellRect(int x, int y) const;

    void processPlayerClick(const QPoint& mousePos);
    void processEnemyTurn();
    void checkWinCondition();

    Unit* findUnitAt(int boardX, int boardY) const;
    bool isAdjacent(const Position& a, const Position& b) const;
    Position findNearestHero(const Position& from) const;

    Ui::MainWindow *ui;
    QTimer *m_gameTimer;
    QElapsedTimer m_frameClock;

    Board m_board;
    std::vector<std::unique_ptr<Unit>> m_units;
    std::vector<std::unique_ptr<Weapon>> m_weapons;

    Unit* m_selectedUnit;
    bool m_isPlayerTurn;
    bool m_gameOver;

    static constexpr int CELL_SIZE = 80;
    static constexpr int BOARD_OFFSET_X = 200;
    static constexpr int BOARD_OFFSET_Y = 80;
    static constexpr int BOARD_PIXEL_SIZE = CELL_SIZE * Board::SIZE;
};

#endif // SYNERA_H
