#ifndef HERO_H
#define HERO_H

#include "unit.h"

class Hero : public Unit {
public:
    Hero(const std::string& name, int hp, int maxHp, int x, int y);
    void attack(Unit& target) override;
};

#endif // HERO_H
