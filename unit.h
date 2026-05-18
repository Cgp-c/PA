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

enum class UnitType { Warrior, Mage, Support, Assassin };

inline int manhattanDist(const Position& a, const Position& b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

class Board;

class Unit {
public:
    static constexpr int MAX_MANA = 5;

    Unit(const std::string& name, int hp, int maxHp, int x, int y, UnitType type,
         int moveSpeed = 30, int attackSpeed = 60, int startMana = 0);
    virtual ~Unit() = default;

    virtual void attack(Unit& target);

    bool isDead() const;
    bool isDisappeared() const;

    void takeDamage(int damage);
    void setDisappeared(bool disappeared);
    void heal(int amount);

    // 法力值
    int getMana() const;
    int getMaxMana() const;
    void gainMana();
    void resetMana();

    // 技能（纯虚，子类各自实现）
    virtual void useSkill(Board& board, std::vector<Unit*>& allUnits) = 0;

    // 燃烧状态
    bool isBurning() const;
    int getBurningTurns() const;
    void applyBurning(int turns);
    void tickBurning();

    std::string getName() const;
    int getHp() const;
    int getMaxHp() const;
    Position getPosition() const;
    Weapon* getEquipment() const;
    UnitType getType() const;

    virtual int getAttackRange() const = 0;
    virtual int getAttackDamage() const = 0;
    virtual int getHealAmount() const { return 0; }
    virtual bool canHeal() const { return false; }

    void setPosition(int x, int y);
    void setEquipment(Weapon* weapon);
    void setHp(int hp);
    void setMaxHp(int maxHp);

    // 速度 / 计时器
    int getMoveSpeed() const;
    int getAttackSpeed() const;
    int getMoveTimer() const;
    int getAttackTimer() const;
    void incrementTimers();
    void resetMoveTimer();
    void resetAttackTimer();

protected:
    std::string m_name;
    int m_hp;
    int m_maxHp;
    Position m_pos;
    Weapon* m_equipment;
    bool m_disappeared;
    UnitType m_type;
    int m_mana;
    bool m_burning;
    int m_burningTurns;
    int m_moveSpeed;
    int m_attackSpeed;
    int m_moveTimer;
    int m_attackTimer;
};

#endif // UNIT_H
