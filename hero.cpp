#include "hero.h"

// 技能的通用实现见 Unit::warriorSkill / mageSkill / supportSkill / assassinSkill

Hero::Hero(const std::string& name, int hp, int maxHp, int x, int y, UnitType type,
           int moveSpeed, int attackSpeed, int startMana)
    : Unit(name, hp, maxHp, x, y, type, moveSpeed, attackSpeed, startMana)
{
}

WarriorHero::WarriorHero(int starLevel, int x, int y)
    : Hero("A-Warrior", BASE_HP * (starLevel / 2 + 1), BASE_HP * (starLevel / 2 + 1),
           x, y, UnitType::Warrior, 60, 120)
{
    m_starLevel = starLevel;
}

MageHero::MageHero(int starLevel, int x, int y)
    : Hero("A-Mage", BASE_HP * (starLevel / 2 + 1), BASE_HP * (starLevel / 2 + 1),
           x, y, UnitType::Mage, 60, 120)
{
    m_starLevel = starLevel;
}

SupportHero::SupportHero(int starLevel, int x, int y)
    : Hero("A-Support", BASE_HP * (starLevel / 2 + 1), BASE_HP * (starLevel / 2 + 1),
           x, y, UnitType::Support, 60, 120)
{
    m_starLevel = starLevel;
}

AssassinHero::AssassinHero(int starLevel, int x, int y)
    : Hero("A-Assassin", BASE_HP * (starLevel / 2 + 1), BASE_HP * (starLevel / 2 + 1),
           x, y, UnitType::Assassin, 60, 80, Unit::BASE_MAX_MANA)
{
    m_starLevel = starLevel;
}
