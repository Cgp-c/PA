#include "hero.h"

// 技能的通用实现见 Unit::warriorSkill / mageSkill / supportSkill / assassinSkill
// 及 hunterSkill / knightSkill / shamanSkill / ultimateSkill。
// 数值全部来自 unitstats 数据表（config/unitstats.json 可调）。

Hero::Hero(const std::string& name, int hp, int maxHp, int x, int y, UnitType type,
           int moveSpeed, int attackSpeed, int startMana)
    : Unit(name, hp, maxHp, x, y, type, moveSpeed, attackSpeed, startMana)
{
}

static std::string heroName(UnitType t)
{
    return std::string("A-") + statsOf(t).name.toStdString();
}

WarriorHero::WarriorHero(int starLevel, int x, int y)
    : Hero(heroName(UnitType::Warrior), baseHpAtStar(UnitType::Warrior, starLevel),
           baseHpAtStar(UnitType::Warrior, starLevel), x, y, UnitType::Warrior,
           statsOf(UnitType::Warrior).moveSpeed, statsOf(UnitType::Warrior).attackSpeed)
{
    m_starLevel = starLevel;
}

MageHero::MageHero(int starLevel, int x, int y)
    : Hero(heroName(UnitType::Mage), baseHpAtStar(UnitType::Mage, starLevel),
           baseHpAtStar(UnitType::Mage, starLevel), x, y, UnitType::Mage,
           statsOf(UnitType::Mage).moveSpeed, statsOf(UnitType::Mage).attackSpeed)
{
    m_starLevel = starLevel;
}

SupportHero::SupportHero(int starLevel, int x, int y)
    : Hero(heroName(UnitType::Support), baseHpAtStar(UnitType::Support, starLevel),
           baseHpAtStar(UnitType::Support, starLevel), x, y, UnitType::Support,
           statsOf(UnitType::Support).moveSpeed, statsOf(UnitType::Support).attackSpeed)
{
    m_starLevel = starLevel;
}

AssassinHero::AssassinHero(int starLevel, int x, int y)
    : Hero(heroName(UnitType::Assassin), baseHpAtStar(UnitType::Assassin, starLevel),
           baseHpAtStar(UnitType::Assassin, starLevel), x, y, UnitType::Assassin,
           statsOf(UnitType::Assassin).moveSpeed, statsOf(UnitType::Assassin).attackSpeed,
           Unit::BASE_MAX_MANA)
{
    m_starLevel = starLevel;
}

// ─── v0.23 新职业 ───────────────────────────────────────────

HunterHero::HunterHero(int starLevel, int x, int y)
    : Hero(heroName(UnitType::Hunter), baseHpAtStar(UnitType::Hunter, starLevel),
           baseHpAtStar(UnitType::Hunter, starLevel), x, y, UnitType::Hunter,
           statsOf(UnitType::Hunter).moveSpeed, statsOf(UnitType::Hunter).attackSpeed)
{
    m_starLevel = starLevel;
}

KnightHero::KnightHero(int starLevel, int x, int y)
    : Hero(heroName(UnitType::Knight), baseHpAtStar(UnitType::Knight, starLevel),
           baseHpAtStar(UnitType::Knight, starLevel), x, y, UnitType::Knight,
           statsOf(UnitType::Knight).moveSpeed, statsOf(UnitType::Knight).attackSpeed)
{
    m_starLevel = starLevel;
}

ShamanHero::ShamanHero(int starLevel, int x, int y)
    : Hero(heroName(UnitType::Shaman), baseHpAtStar(UnitType::Shaman, starLevel),
           baseHpAtStar(UnitType::Shaman, starLevel), x, y, UnitType::Shaman,
           statsOf(UnitType::Shaman).moveSpeed, statsOf(UnitType::Shaman).attackSpeed)
{
    m_starLevel = starLevel;
}

// 终极角色：动态数值由合成方（或默认档）传入；星级恒满、永不可再升级
UltimateHero::UltimateHero(int hp, int atk, int x, int y)
    : Hero(std::string("A-") + QString("Ultimate").toStdString(), hp, hp,
           x, y, UnitType::Ultimate,
           ultimateStats().moveSpeed, ultimateStats().attackSpeed)
    , m_ultimateAtk(atk)
{
    m_starLevel = 6;   // 显示三星满星；tryStarUp 显式拒绝其参与任何合成
}
