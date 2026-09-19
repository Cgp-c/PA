#ifndef ENEMY_H
#define ENEMY_H

#include "unit.h"

class Enemy : public Unit {
public:
    Enemy(const std::string& name, int hp, int maxHp, int x, int y, UnitType type,
          int moveSpeed, int attackSpeed, int startMana = 0,
          int maxMana = Unit::BASE_MAX_MANA, int maxMana2 = 0);
    bool isHeroSide() const override { return false; }
};

class WarriorEnemy : public Enemy {
public:
    WarriorEnemy(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return 1; }
    int getAttackDamage() const override { return 20 * (m_starLevel / 2 + 1); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { warriorSkill(board, allUnits); }
};


class MageEnemy : public Enemy {
public:
    MageEnemy(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return 4; }
    int getAttackDamage() const override { return 10 * (m_starLevel / 2 + 1); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { mageSkill(board, allUnits); }
};

class SupportEnemy : public Enemy {
public:
    SupportEnemy(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return 2; }
    int getAttackDamage() const override { return 0; }
    int getHealAmount() const override { return 20 * (m_starLevel / 2 + 1); }
    bool canHeal() const override { return true; }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { supportSkill(board, allUnits); }
};

class AssassinEnemy : public Enemy {
public:
    AssassinEnemy(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return 1; }
    int getAttackDamage() const override { return 50 * (m_starLevel / 2 + 1); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { assassinSkill(board, allUnits); }
};

class BossEnemy : public Enemy {
public:
    BossEnemy(int x = 0, int y = 0);
    int getAttackRange() const override { return 1; }
    int getAttackDamage() const override { return 20; }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override;
    void useSkill2(Board& board, std::vector<Unit*>& allUnits) override;
};

// 阵营判定：替代散落各处的 dynamic_cast 样板
inline bool isEnemySide(const Unit* u) { return u && dynamic_cast<const Enemy*>(u) != nullptr; }

#endif // ENEMY_H
