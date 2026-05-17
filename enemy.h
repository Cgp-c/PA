#ifndef ENEMY_H
#define ENEMY_H

#include "unit.h"

class Enemy : public Unit {
public:
    Enemy(const std::string& name, int hp, int maxHp, int x, int y);
    void attack(Unit& target) override;
};

#endif // ENEMY_H
