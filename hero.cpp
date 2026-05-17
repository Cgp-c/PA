#include "hero.h"

Hero::Hero(const std::string& name, int hp, int maxHp, int x, int y, UnitType type)
    : Unit(name, hp, maxHp, x, y, type)
{
}

WarriorHero::WarriorHero(int x, int y)
    : Hero("A-Warrior", 100, 100, x, y, UnitType::Warrior)
{
}

MageHero::MageHero(int x, int y)
    : Hero("A-Mage", 50, 50, x, y, UnitType::Mage)
{
}

SupportHero::SupportHero(int x, int y)
    : Hero("A-Support", 80, 80, x, y, UnitType::Support)
{
}
