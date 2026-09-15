#include "enemy.h"

// 四职业技能的通用实现见 Unit::warriorSkill / mageSkill / supportSkill / assassinSkill；
// 本文件只保留 Boss 专属技能

Enemy::Enemy(const std::string& name, int hp, int maxHp, int x, int y, UnitType type,
             int moveSpeed, int attackSpeed, int startMana,
             int maxMana, int maxMana2)
    : Unit(name, hp, maxHp, x, y, type, moveSpeed, attackSpeed, startMana, maxMana, maxMana2)
{
}

WarriorEnemy::WarriorEnemy(int starLevel, int x, int y)
    : Enemy("E-Warrior", 100 * (starLevel / 2 + 1), 100 * (starLevel / 2 + 1),
            x, y, UnitType::Warrior, 60, 120)
{
    m_starLevel = starLevel;
}

MageEnemy::MageEnemy(int starLevel, int x, int y)
    : Enemy("E-Mage", 50 * (starLevel / 2 + 1), 50 * (starLevel / 2 + 1),
            x, y, UnitType::Mage, 60, 120)
{
    m_starLevel = starLevel;
}

SupportEnemy::SupportEnemy(int starLevel, int x, int y)
    : Enemy("E-Support", 80 * (starLevel / 2 + 1), 80 * (starLevel / 2 + 1),
            x, y, UnitType::Support, 60, 120)
{
    m_starLevel = starLevel;
}

AssassinEnemy::AssassinEnemy(int starLevel, int x, int y)
    : Enemy("E-Assassin", 15 * (starLevel / 2 + 1), 15 * (starLevel / 2 + 1),
            x, y, UnitType::Assassin, 60, 80, Unit::BASE_MAX_MANA)
{
    m_starLevel = starLevel;
}

// maxMana2 = 100 = 5 点法力（MANA_PER_POINT=20），触发进阶技能
BossEnemy::BossEnemy(int x, int y)
    : Enemy("E-Boss", 500, 500, x, y, UnitType::Boss, 120, 120, 0, 60, 100)
{
}

// ─── Boss 基础技能（3 点法力）：攻击全体英雄，各造成 10 伤害 ──

void BossEnemy::useSkill(Board& board, std::vector<Unit*>& allUnits)
{
    (void)board;
    for (Unit* u : allUnits) {
        if (u == this || u->isDead() || u->isDisappeared()) continue;
        if (!isOpponentOf(u)) continue;
        u->takeDamage(10);
    }
}

// ─── Boss 进阶技能（5 点法力）：攻击米字型四列路径上的全体英雄，各造成 30 伤害 ──

void BossEnemy::useSkill2(Board& board, std::vector<Unit*>& allUnits)
{
    (void)board;
    for (Unit* u : allUnits) {
        if (u == this || u->isDead() || u->isDisappeared()) continue;
        if (!isOpponentOf(u)) continue;
        Position up = u->getPosition();
        // 米字型：同行、同列、同主对角线、同副对角线
        if (up.x == m_pos.x || up.y == m_pos.y
            || (up.x - up.y) == (m_pos.x - m_pos.y)
            || (up.x + up.y) == (m_pos.x + m_pos.y))
        {
            u->takeDamage(30);
        }
    }
}
