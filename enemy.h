#ifndef ENEMY_H
#define ENEMY_H

#include "unit.h"

class Enemy : public Unit {
public:
    Enemy(const std::string& name, int hp, int maxHp, int x, int y, UnitType type,
          int moveSpeed, int attackSpeed, int startMana = 0,
          int maxMana = Unit::MAX_MANA, int maxMana2 = 0);
};

class WarriorEnemy : public Enemy {
public:
    WarriorEnemy(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return 1; }
    int getAttackDamage() const override { return 20 * (m_starLevel + 1); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override;
};

class MageEnemy : public Enemy {
public:
    MageEnemy(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return 4; }
    int getAttackDamage() const override { return 10 * (m_starLevel + 1); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override;
};

class SupportEnemy : public Enemy {
public:
    SupportEnemy(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return 1; }
    int getAttackDamage() const override { return 0; }
    int getHealAmount() const override { return 20 * (m_starLevel + 1); }
    bool canHeal() const override { return true; }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override;
};

class AssassinEnemy : public Enemy {
public:
    AssassinEnemy(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return 1; }
    int getAttackDamage() const override { return 50 * (m_starLevel + 1); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override;
};

class BossEnemy : public Enemy {
public:
    BossEnemy(int x = 0, int y = 0);
    int getAttackRange() const override { return 1; }
    int getAttackDamage() const override { return 20; }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override;
    void useSkill2(Board& board, std::vector<Unit*>& allUnits) override;
};

#endif // ENEMY_H
