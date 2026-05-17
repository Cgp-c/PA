#ifndef WEAPON_H
#define WEAPON_H

#include <string>

class Weapon {
public:
    Weapon(const std::string& name, int damage);

    std::string getName() const;
    int getDamage() const;

private:
    std::string m_name;
    int m_damage;
};

#endif // WEAPON_H
