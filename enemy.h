#ifndef ENEMY_H
#define ENEMY_H

#include "unit.h"
#include "unitstats.h"

// 敌方侧基类：数值与英雄侧共用同一份 unitstats 数据表（根除双份硬编码漂移）

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
    int getAttackRange() const override { return statsOf(UnitType::Warrior).range; }
    int getAttackDamage() const override { return statsOf(UnitType::Warrior).atk * (m_starLevel / 2 + 1); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { warriorSkill(board, allUnits); }
};


class MageEnemy : public Enemy {
public:
    MageEnemy(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return statsOf(UnitType::Mage).range; }
    int getAttackDamage() const override { return statsOf(UnitType::Mage).atk * (m_starLevel / 2 + 1); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { mageSkill(board, allUnits); }
};

class SupportEnemy : public Enemy {
public:
    SupportEnemy(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return statsOf(UnitType::Support).range; }
    int getAttackDamage() const override { return 0; }
    int getHealAmount() const override { return statsOf(UnitType::Support).heal * (m_starLevel / 2 + 1); }
    bool canHeal() const override { return true; }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { supportSkill(board, allUnits); }
};

class AssassinEnemy : public Enemy {
public:
    AssassinEnemy(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return statsOf(UnitType::Assassin).range; }
    int getAttackDamage() const override { return statsOf(UnitType::Assassin).atk * (m_starLevel / 2 + 1); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { assassinSkill(board, allUnits); }
};

class BossEnemy : public Enemy {
public:
    BossEnemy(int x = 0, int y = 0);
    int getAttackRange() const override { return statsOf(UnitType::Boss).range; }
    int getAttackDamage() const override { return statsOf(UnitType::Boss).atk; }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override;
    void useSkill2(Board& board, std::vector<Unit*>& allUnits) override;
};

// ─── v0.23 新职业（敌方侧）─────────────────────────────────

class HunterEnemy : public Enemy {
public:
    HunterEnemy(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return statsOf(UnitType::Hunter).range; }
    int getAttackDamage() const override { return statsOf(UnitType::Hunter).atk * (m_starLevel / 2 + 1); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { hunterSkill(board, allUnits); }
};

class KnightEnemy : public Enemy {
public:
    KnightEnemy(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return statsOf(UnitType::Knight).range; }
    int getAttackDamage() const override { return statsOf(UnitType::Knight).atk * (m_starLevel / 2 + 1); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { knightSkill(board, allUnits); }
};

class ShamanEnemy : public Enemy {
public:
    ShamanEnemy(int starLevel = 0, int x = 0, int y = 0);
    int getAttackRange() const override { return statsOf(UnitType::Shaman).range; }
    int getAttackDamage() const override { return statsOf(UnitType::Shaman).atk * (m_starLevel / 2 + 1); }
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { shamanSkill(board, allUnits); }
};

class UltimateEnemy : public Enemy {
public:
    // atk 默认取数据表默认档（自定义难度直选时）
    explicit UltimateEnemy(int atk = ultimateStats().defaultAtk, int x = 0, int y = 0);
    int getAttackRange() const override { return ultimateStats().range; }
    int getAttackDamage() const override { return m_ultimateAtk; }
    void setDynamicAtk(int atk) { m_ultimateAtk = atk; }   // 回放/联机重建时恢复
    void useSkill(Board& board, std::vector<Unit*>& allUnits) override { ultimateSkill(board, allUnits); }
private:
    int m_ultimateAtk;
};

// 阵营判定：替代散落各处的 dynamic_cast 样板
inline bool isEnemySide(const Unit* u) { return u && dynamic_cast<const Enemy*>(u) != nullptr; }

#endif // ENEMY_H
