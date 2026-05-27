#ifndef HERO_H
#define HERO_H

#include "unit.h"

class Hero : public Unit {
public:
    Hero(const std::string& name, int hp, int maxHp, int x, int y, UnitType type,
         int moveSpeed, int attackSpeed, int startMana = 0);
};

class WarriorHero : public Hero {
    static constexpr int BASE_HP = 100;
    static constexpr int BASE_ATK = 20;
    static constexpr int BASE_SKILL_DMG = 80;
public:
    WarriorHero(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return 1 + getEquipBonusRange(); }
    int getAttackDamage() const override { return BASE_ATK * (m_starLevel / 2 + 1); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override;
};

class MageHero : public Hero {
    static constexpr int BASE_HP = 50;
    static constexpr int BASE_ATK = 10;
public:
    MageHero(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return 4 + getEquipBonusRange(); }
    int getAttackDamage() const override { return BASE_ATK * (m_starLevel / 2 + 1); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override;
};

class SupportHero : public Hero {
    static constexpr int BASE_HP = 80;
    static constexpr int BASE_HEAL = 20;
    static constexpr int BASE_SKILL_HEAL = 30;
public:
    SupportHero(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return 2 + getEquipBonusRange(); }
    int getAttackDamage() const override { return 0; }
    int getHealAmount() const override { return BASE_HEAL * (m_starLevel / 2 + 1); }
    bool canHeal() const override { return true; }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override;
};

class AssassinHero : public Hero {
    static constexpr int BASE_HP = 15;
    static constexpr int BASE_ATK = 50;
    static constexpr int BASE_SKILL_DMG = 80;
public:
    AssassinHero(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return 1 + getEquipBonusRange(); }
    int getAttackDamage() const override { return BASE_ATK * (m_starLevel / 2 + 1); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override;
};

#endif // HERO_H
