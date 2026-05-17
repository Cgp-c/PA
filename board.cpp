#include "board.h"
#include <algorithm>

Board::Board()
{
    for (int y = 0; y < SIZE; ++y)
        for (int x = 0; x < SIZE; ++x)
            m_grid[y][x] = nullptr;
}

bool Board::placeUnit(Unit* unit, int x, int y)
{
    if (!isValidPosition(x, y) || isOccupied(x, y))
        return false;
    m_grid[y][x] = unit;
    unit->setPosition(x, y);
    return true;
}

Unit* Board::getUnitAt(int x, int y) const
{
    if (!isValidPosition(x, y))
        return nullptr;
    return m_grid[y][x];
}

bool Board::isOccupied(int x, int y) const
{
    return getUnitAt(x, y) != nullptr;
}

void Board::removeUnit(int x, int y)
{
    if (isValidPosition(x, y))
        m_grid[y][x] = nullptr;
}

bool Board::isValidPosition(int x, int y) const
{
    return x >= 0 && x < SIZE && y >= 0 && y < SIZE;
}

bool Board::isPlayerHalf(int y) const
{
    return y >= SIZE / 2;
}

bool Board::isEnemyHalf(int y) const
{
    return y < SIZE / 2;
}

void Board::clear()
{
    for (int y = 0; y < SIZE; ++y)
        for (int x = 0; x < SIZE; ++x)
            m_grid[y][x] = nullptr;
}
