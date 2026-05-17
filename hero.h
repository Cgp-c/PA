#ifndef HERO_H
#define HERO_H

#include "unit.h"

class Hero : public Unit {
public:
    Hero(const std::string& name, int hp, int maxHp, int x, int y, UnitType type);
};

class WarriorHero : public Hero {
public:
    WarriorHero(int x = 0, int y = 0);
    int getAttackRange() const override { return 1; }
    int getAttackDamage() const override { return 20; }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override;
};

class MageHero : public Hero {
public:
    MageHero(int x = 0, int y = 0);
    int getAttackRange() const override { return 4; }
    int getAttackDamage() const override { return 10; }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override;
};

class SupportHero : public Hero {
public:
    SupportHero(int x = 0, int y = 0);
    int getAttackRange() const override { return 1; }
    int getAttackDamage() const override { return 0; }
    int getHealAmount() const override { return 20; }
    bool canHeal() const override { return true; }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override;
};

#endif // HERO_H
