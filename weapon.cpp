#include "weapon.h"

Weapon::Weapon(const std::string& name, int damage)
    : m_name(name), m_damage(damage)
{
}

std::string Weapon::getName() const { return m_name; }
int Weapon::getDamage() const { return m_damage; }
