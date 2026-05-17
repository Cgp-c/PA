#ifndef ENEMY_H
#define ENEMY_H

#include "unit.h"

class Enemy : public Unit {
public:
    Enemy(const std::string& name, int hp, int maxHp, int x, int y, UnitType type);
};

class WarriorEnemy : public Enemy {
public:
    WarriorEnemy(int x = 0, int y = 0);
    int getAttackRange() const override { return 1; }
    int getAttackDamage() const override { return 20; }
};

class MageEnemy : public Enemy {
public:
    MageEnemy(int x = 0, int y = 0);
    int getAttackRange() const override { return 4; }
    int getAttackDamage() const override { return 10; }
};

class SupportEnemy : public Enemy {
public:
    SupportEnemy(int x = 0, int y = 0);
    int getAttackRange() const override { return 1; }
    int getAttackDamage() const override { return 0; }
    int getHealAmount() const override { return 20; }
    bool canHeal() const override { return true; }
};

#endif // ENEMY_H
