#include "enemy.h"

Enemy::Enemy(const std::string& name, int hp, int maxHp, int x, int y)
    : Unit(name, hp, maxHp, x, y)
{
}

void Enemy::attack(Unit& target)
{
    int damage = 8;
    if (m_equipment)
        damage = m_equipment->getDamage();
    target.takeDamage(damage);
    if (target.isDead())
        target.setDisappeared(true);
}
