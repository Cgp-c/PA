#ifndef UNIT_H
#define UNIT_H

#include <string>
#include "weapon.h"

struct Position {
    int x;
    int y;
    Position(int x = 0, int y = 0) : x(x), y(y) {}
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

enum class UnitType { Warrior, Mage, Support };

inline int manhattanDist(const Position& a, const Position& b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

class Unit {
public:
    Unit(const std::string& name, int hp, int maxHp, int x, int y, UnitType type);
    virtual ~Unit() = default;

    virtual void attack(Unit& target);

    bool isDead() const;
    bool isDisappeared() const;

    void takeDamage(int damage);
    void setDisappeared(bool disappeared);
    void heal(int amount);

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

protected:
    std::string m_name;
    int m_hp;
    int m_maxHp;
    Position m_pos;
    Weapon* m_equipment;
    bool m_disappeared;
    UnitType m_type;
};

#endif // UNIT_H
