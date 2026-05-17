#ifndef BOARD_H
#define BOARD_H

#include "unit.h"

class Board {
public:
    static constexpr int SIZE = 8;

    Board();

    bool placeUnit(Unit* unit, int x, int y);
    Unit* getUnitAt(int x, int y) const;
    bool isOccupied(int x, int y) const;
    void removeUnit(int x, int y);
    bool isValidPosition(int x, int y) const;
    bool isPlayerHalf(int y) const;
    bool isEnemyHalf(int y) const;
    void clear();

private:
    Unit* m_grid[SIZE][SIZE];
};

#endif // BOARD_H
