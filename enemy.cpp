#include "enemy.h"

// 四职业 + 新职业技能的通用实现见 Unit 的技能成员；
// 本文件只保留构造与 Boss 专属技能。数值全部来自 unitstats 数据表。

Enemy::Enemy(const std::string& name, int hp, int maxHp, int x, int y, UnitType type,
             int moveSpeed, int attackSpeed, int startMana,
             int maxMana, int maxMana2)
    : Unit(name, hp, maxHp, x, y, type, moveSpeed, attackSpeed, startMana, maxMana, maxMana2)
{
}

static std::string enemyName(UnitType t)
{
    return std::string("E-") + statsOf(t).name.toStdString();
}

WarriorEnemy::WarriorEnemy(int starLevel, int x, int y)
    : Enemy(enemyName(UnitType::Warrior), baseHpAtStar(UnitType::Warrior, starLevel),
            baseHpAtStar(UnitType::Warrior, starLevel), x, y, UnitType::Warrior,
            statsOf(UnitType::Warrior).moveSpeed, statsOf(UnitType::Warrior).attackSpeed)
{
    m_starLevel = starLevel;
}

MageEnemy::MageEnemy(int starLevel, int x, int y)
    : Enemy(enemyName(UnitType::Mage), baseHpAtStar(UnitType::Mage, starLevel),
            baseHpAtStar(UnitType::Mage, starLevel), x, y, UnitType::Mage,
            statsOf(UnitType::Mage).moveSpeed, statsOf(UnitType::Mage).attackSpeed)
{
    m_starLevel = starLevel;
}

SupportEnemy::SupportEnemy(int starLevel, int x, int y)
    : Enemy(enemyName(UnitType::Support), baseHpAtStar(UnitType::Support, starLevel),
            baseHpAtStar(UnitType::Support, starLevel), x, y, UnitType::Support,
            statsOf(UnitType::Support).moveSpeed, statsOf(UnitType::Support).attackSpeed)
{
    m_starLevel = starLevel;
}

AssassinEnemy::AssassinEnemy(int starLevel, int x, int y)
    : Enemy(enemyName(UnitType::Assassin), baseHpAtStar(UnitType::Assassin, starLevel),
            baseHpAtStar(UnitType::Assassin, starLevel), x, y, UnitType::Assassin,
            statsOf(UnitType::Assassin).moveSpeed, statsOf(UnitType::Assassin).attackSpeed,
            Unit::BASE_MAX_MANA)
{
    m_starLevel = starLevel;
}

// maxMana2 = 100 = 5 点法力（MANA_PER_POINT=20），触发进阶技能
BossEnemy::BossEnemy(int x, int y)
    : Enemy("E-Boss", statsOf(UnitType::Boss).hp, statsOf(UnitType::Boss).hp,
            x, y, UnitType::Boss,
            statsOf(UnitType::Boss).moveSpeed, statsOf(UnitType::Boss).attackSpeed,
            0, 60, 100)
{
}

// ─── v0.23 新职业（敌方侧）─────────────────────────────────

HunterEnemy::HunterEnemy(int starLevel, int x, int y)
    : Enemy(enemyName(UnitType::Hunter), baseHpAtStar(UnitType::Hunter, starLevel),
            baseHpAtStar(UnitType::Hunter, starLevel), x, y, UnitType::Hunter,
            statsOf(UnitType::Hunter).moveSpeed, statsOf(UnitType::Hunter).attackSpeed)
{
    m_starLevel = starLevel;
}

KnightEnemy::KnightEnemy(int starLevel, int x, int y)
    : Enemy(enemyName(UnitType::Knight), baseHpAtStar(UnitType::Knight, starLevel),
            baseHpAtStar(UnitType::Knight, starLevel), x, y, UnitType::Knight,
            statsOf(UnitType::Knight).moveSpeed, statsOf(UnitType::Knight).attackSpeed)
{
    m_starLevel = starLevel;
}

ShamanEnemy::ShamanEnemy(int starLevel, int x, int y)
    : Enemy(enemyName(UnitType::Shaman), baseHpAtStar(UnitType::Shaman, starLevel),
            baseHpAtStar(UnitType::Shaman, starLevel), x, y, UnitType::Shaman,
            statsOf(UnitType::Shaman).moveSpeed, statsOf(UnitType::Shaman).attackSpeed)
{
    m_starLevel = starLevel;
}

UltimateEnemy::UltimateEnemy(int atk, int x, int y)
    : Enemy("E-Ultimate", ultimateStats().defaultHp, ultimateStats().defaultHp,
            x, y, UnitType::Ultimate,
            ultimateStats().moveSpeed, ultimateStats().attackSpeed)
    , m_ultimateAtk(atk)
{
    m_starLevel = 6;
}

// ─── Boss 基础技能（3 点法力）：攻击全体英雄 ──
void BossEnemy::useSkill(Board& board, std::vector<Unit*>& allUnits)
{
    (void)board;
    for (Unit* u : allUnits) {
        if (u == this || u->isDead() || u->isDisappeared()) continue;
        if (!isOpponentOf(u)) continue;
        u->takeDamage(statsOf(UnitType::Boss).skillDmg / 3);
    }
}

// ─── Boss 进阶技能（5 点法力）：米字路径全体英雄 ──
void BossEnemy::useSkill2(Board& board, std::vector<Unit*>& allUnits)
{
    (void)board;
    for (Unit* u : allUnits) {
        if (u == this || u->isDead() || u->isDisappeared()) continue;
        if (!isOpponentOf(u)) continue;
        int dx = std::abs(u->getPosition().x - getPosition().x);
        int dy = std::abs(u->getPosition().y - getPosition().y);
        if (dx == dy || dx == 0 || dy == 0)
            u->takeDamage(statsOf(UnitType::Boss).skillDmg);
    }
}
