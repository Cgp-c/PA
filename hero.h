#ifndef HERO_H
#define HERO_H

#include "unit.h"
#include "unitstats.h"

// 英雄侧基类：数值全部来自 unitstats 数据表（config/unitstats.json 可调）

class Hero : public Unit {
public:
    Hero(const std::string& name, int hp, int maxHp, int x, int y, UnitType type,
         int moveSpeed, int attackSpeed, int startMana = 0);
    bool isHeroSide() const override { return true; }
};

class WarriorHero : public Hero {
public:
    WarriorHero(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return statsOf(UnitType::Warrior).range + getEquipBonusRange() + getBondRangeBonus(); }
    int getAttackDamage() const override { return statsOf(UnitType::Warrior).atk * (m_starLevel / 2 + 1) + getBondAtkBonus(); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { warriorSkill(board, allUnits); }
};

class MageHero : public Hero {
public:
    MageHero(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return statsOf(UnitType::Mage).range + getEquipBonusRange() + getBondRangeBonus(); }
    int getAttackDamage() const override { return statsOf(UnitType::Mage).atk * (m_starLevel / 2 + 1) + getBondAtkBonus(); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { mageSkill(board, allUnits); }
};

class SupportHero : public Hero {
public:
    SupportHero(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return statsOf(UnitType::Support).range + getEquipBonusRange() + getBondRangeBonus(); }
    int getAttackDamage() const override { return 0; }
    int getHealAmount() const override { return static_cast<int>((statsOf(UnitType::Support).heal * (m_starLevel / 2 + 1) + getEquipBonusHeal()) * getBondHealMult()); }
    bool canHeal() const override { return true; }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { supportSkill(board, allUnits); }
};

class AssassinHero : public Hero {
public:
    AssassinHero(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return statsOf(UnitType::Assassin).range + getEquipBonusRange() + getBondRangeBonus(); }
    int getAttackDamage() const override { return statsOf(UnitType::Assassin).atk * (m_starLevel / 2 + 1) + getBondAtkBonus(); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { assassinSkill(board, allUnits); }
};

// ─── v0.23 新职业 ───────────────────────────────────────────

class HunterHero : public Hero {
public:
    HunterHero(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return statsOf(UnitType::Hunter).range + getEquipBonusRange() + getBondRangeBonus(); }
    int getAttackDamage() const override { return statsOf(UnitType::Hunter).atk * (m_starLevel / 2 + 1) + getBondAtkBonus(); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { hunterSkill(board, allUnits); }
};

class KnightHero : public Hero {
public:
    KnightHero(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return statsOf(UnitType::Knight).range + getEquipBonusRange() + getBondRangeBonus(); }
    int getAttackDamage() const override { return statsOf(UnitType::Knight).atk * (m_starLevel / 2 + 1) + getBondAtkBonus(); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { knightSkill(board, allUnits); }
};

class ShamanHero : public Hero {
public:
    ShamanHero(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return statsOf(UnitType::Shaman).range + getEquipBonusRange() + getBondRangeBonus(); }
    int getAttackDamage() const override { return statsOf(UnitType::Shaman).atk * (m_starLevel / 2 + 1) + getBondAtkBonus(); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { shamanSkill(board, allUnits); }
};

// 终极角色：HP/攻击由三个素材三星基础值之和 × 比例动态生成（合成时计算传入）
class UltimateHero : public Hero {
public:
    explicit UltimateHero(int hp, int atk, int x = 0, int y = 0);
    int getAttackRange() const override { return ultimateStats().range + getEquipBonusRange() + getBondRangeBonus(); }
    int getAttackDamage() const override { return m_ultimateAtk + getBondAtkBonus(); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { ultimateSkill(board, allUnits); }
    void setDynamicAtk(int atk) { m_ultimateAtk = atk; }   // 回放/联机重建时恢复动态攻击
private:
    int m_ultimateAtk;
};

// 阵营判定：替代散落各处的 dynamic_cast 样板
inline bool isHeroSide(const Unit* u) { return u && dynamic_cast<const Hero*>(u) != nullptr; }

#endif // HERO_H
