#ifndef HERO_H
#define HERO_H

#include "unit.h"

class Hero : public Unit {
public:
    Hero(const std::string& name, int hp, int maxHp, int x, int y, UnitType type,
         int moveSpeed, int attackSpeed, int startMana = 0);
    bool isHeroSide() const override { return true; }
};

class WarriorHero : public Hero {
public:
    static constexpr int BASE_HP = 100;
    static constexpr int BASE_ATK = 20;
    WarriorHero(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return 1 + getEquipBonusRange() + getBondRangeBonus(); }
    int getAttackDamage() const override { return BASE_ATK * (m_starLevel / 2 + 1) + getBondAtkBonus(); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { warriorSkill(board, allUnits); }
};

class MageHero : public Hero {
public:
    static constexpr int BASE_HP = 50;
    static constexpr int BASE_ATK = 10;
    MageHero(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return 4 + getEquipBonusRange() + getBondRangeBonus(); }
    int getAttackDamage() const override { return BASE_ATK * (m_starLevel / 2 + 1) + getBondAtkBonus(); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { mageSkill(board, allUnits); }
};

class SupportHero : public Hero {
public:
    static constexpr int BASE_HP = 80;
    static constexpr int BASE_HEAL = 20;
    SupportHero(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return 2 + getEquipBonusRange() + getBondRangeBonus(); }
    int getAttackDamage() const override { return 0; }
    int getHealAmount() const override { return static_cast<int>((BASE_HEAL * (m_starLevel / 2 + 1) + getEquipBonusHeal()) * getBondHealMult()); }
    bool canHeal() const override { return true; }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { supportSkill(board, allUnits); }
};

class AssassinHero : public Hero {
public:
    static constexpr int BASE_HP = 15;
    static constexpr int BASE_ATK = 50;
    AssassinHero(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return 1 + getEquipBonusRange() + getBondRangeBonus(); }
    int getAttackDamage() const override { return BASE_ATK * (m_starLevel / 2 + 1) + getBondAtkBonus(); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { assassinSkill(board, allUnits); }
};

// 阵营判定：替代散落各处的 dynamic_cast 样板
inline bool isHeroSide(const Unit* u) { return u && dynamic_cast<const Hero*>(u) != nullptr; }

#endif // HERO_H
