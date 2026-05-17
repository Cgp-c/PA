#include "enemy.h"

Enemy::Enemy(const std::string& name, int hp, int maxHp, int x, int y, UnitType type)
    : Unit(name, hp, maxHp, x, y, type)
{
}

WarriorEnemy::WarriorEnemy(int x, int y)
    : Enemy("E-Warrior", 100, 100, x, y, UnitType::Warrior)
{
}

MageEnemy::MageEnemy(int x, int y)
    : Enemy("E-Mage", 50, 50, x, y, UnitType::Mage)
{
}

SupportEnemy::SupportEnemy(int x, int y)
    : Enemy("E-Support", 80, 80, x, y, UnitType::Support)
{
}
