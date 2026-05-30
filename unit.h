#ifndef UNIT_H
#define UNIT_H

#include <string>
#include <vector>
#include "weapon.h"

struct Position {
    int x;
    int y;
    Position(int x = 0, int y = 0) : x(x), y(y) {}
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

enum class UnitType { Warrior, Mage, Support, Assassin, Boss };

inline int manhattanDist(const Position& a, const Position& b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

class Board;

class Unit {
public:
    static constexpr int BASE_MAX_MANA = 100;
    static constexpr int MANA_PER_POINT = 20;

    Unit(const std::string& name, int hp, int maxHp, int x, int y, UnitType type,
         int moveSpeed = 30, int attackSpeed = 60, int startMana = 0,
         int maxMana = BASE_MAX_MANA, int maxMana2 = 0);
    virtual ~Unit() = default;

    virtual int attack(Unit& target);

    bool isDead() const;
    bool isDisappeared() const;

    void takeDamage(int damage);
    void setDisappeared(bool disappeared);
    int heal(int amount);

    // 法力值
    int getMana() const;
    int getMaxMana() const;
    void setMana(int mana);
    void gainMana();
    void resetMana();

    // 第二法力值（Boss 进阶技能）
    int getMana2() const;
    int getMaxMana2() const;
    void gainMana2();
    void resetMana2();

    // 技能（纯虚，子类各自实现）
    virtual void useSkill(Board& board, std::vector<Unit*>& allUnits) = 0;
    virtual void useSkill2(Board& board, std::vector<Unit*>& allUnits);

    // 燃烧状态
    bool isBurning() const;
    int getBurningTurns() const;
    void applyBurning(int turns);
    void tickBurning();

    std::string getName() const;
    int getHp() const;
    int getMaxHp() const;
    Position getPosition() const;
    UnitType getType() const;

    virtual int getAttackRange() const = 0;
    virtual int getAttackDamage() const = 0;
    virtual int getHealAmount() const { return 0; }
    virtual bool canHeal() const { return false; }

    int getStarLevel() const;
    void setStarLevel(int level);

    void setPosition(int x, int y);
    void setHp(int hp);
    void setMaxHp(int maxHp);

    // 装备系统
    bool equip(Weapon* weapon);
    void unequip(EquipType type);
    Weapon* getEquip(EquipType type) const;
    int getMaxEquipSlots() const;
    int getEquippedCount() const;

    int getEquipBonusDamage() const;
    int getEquipBonusHp() const;
    double getEquipSpeedMultiplier() const;
    double getEquipManaCapMultiplier() const;
    int getEquipBonusRange() const;

    // 速度 / 计时器
    int getMoveSpeed() const;
    int getAttackSpeed() const;
    int getMoveTimer() const;
    int getAttackTimer() const;
    void incrementTimers();
    void resetMoveTimer();
    void resetAttackTimer();

    // 羁绊效果
    void resetBondEffects();
    void applyBondHpMult(double mult);
    void applyBondHealMult(double mult);
    void applyBondRangeBonus(int bonus);
    void applyBondManaMod(int mod);
    void applyBondAtkBonus(int bonus);
    void revertBondHpMult(double mult);
    void revertBondHealMult(double mult);
    void revertBondRangeBonus(int bonus);
    void revertBondManaMod(int mod);
    void revertBondAtkBonus(int bonus);

    double getBondHpMult() const { return m_bondHpMult; }
    double getBondHealMult() const { return m_bondHealMult; }
    int getBondRangeBonus() const { return m_bondRangeBonus; }
    int getBondManaMod() const { return m_bondManaMod; }
    int getBondAtkBonus() const { return m_bondAtkBonus; }

    void setClone(bool c) { m_isClone = c; }
    bool isClone() const { return m_isClone; }

protected:
    std::string m_name;
    int m_hp;
    int m_maxHp;
    Position m_pos;
    Weapon* m_equipment[static_cast<int>(EquipType::COUNT)];
    bool m_disappeared;
    UnitType m_type;
    int m_starLevel;
    int m_mana;
    int m_maxMana;
    int m_mana2;
    int m_maxMana2;
    bool m_burning;
    int m_burningTurns;
    int m_moveSpeed;
    int m_attackSpeed;
    int m_moveTimer;
    int m_attackTimer;

    // 羁绊效果
    double m_bondHpMult = 1.0;
    double m_bondHealMult = 1.0;
    int m_bondRangeBonus = 0;
    int m_bondManaMod = 0;
    int m_bondAtkBonus = 0;
    bool m_isClone = false;
};

#endif // UNIT_H
