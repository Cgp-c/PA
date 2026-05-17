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

class Unit {
public:
    Unit(const std::string& name, int hp, int maxHp, int x, int y);
    virtual ~Unit() = default;

    virtual void attack(Unit& target) = 0;

    bool isDead() const;
    bool isDisappeared() const;

    void takeDamage(int damage);
    void setDisappeared(bool disappeared);

    std::string getName() const;
    int getHp() const;
    int getMaxHp() const;
    Position getPosition() const;
    Weapon* getEquipment() const;

    void setPosition(int x, int y);
    void setEquipment(Weapon* weapon);
    void setHp(int hp);

protected:
    std::string m_name;
    int m_hp;
    int m_maxHp;
    Position m_pos;
    Weapon* m_equipment;
    bool m_disappeared;
};

#endif // UNIT_H
