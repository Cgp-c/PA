#include "enemy.h"
#include "hero.h"
#include "board.h"
#include <algorithm>

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

// ─── 战士技能：对最近英雄造成 80 伤害 ──────────────────────

void WarriorEnemy::useSkill(Board& board, std::vector<Unit*>& allUnits)
{
    Unit* best = nullptr;
    int bestDist = 999;
    for (Unit* u : allUnits) {
        if (u == this || u->isDead() || u->isDisappeared()) continue;
        if (dynamic_cast<Hero*>(u) == nullptr) continue;
        int d = manhattanDist(m_pos, u->getPosition());
        if (d < bestDist) { bestDist = d; best = u; }
    }
    if (best) {
        best->takeDamage(80);
        if (best->isDead())
            board.removeUnit(best->getPosition().x, best->getPosition().y);
    }
}

// ─── 法师技能：周围 3×3 英雄全体燃烧 4 回合 ─────────────────

void MageEnemy::useSkill(Board& board, std::vector<Unit*>& allUnits)
{
    (void)board;
    for (Unit* u : allUnits) {
        if (u == this || u->isDead() || u->isDisappeared()) continue;
        if (dynamic_cast<Hero*>(u) == nullptr) continue;
        int dx = std::abs(u->getPosition().x - m_pos.x);
        int dy = std::abs(u->getPosition().y - m_pos.y);
        if (dx <= 1 && dy <= 1) {
            u->applyBurning(4);
        }
    }
}

// ─── 辅助技能：全场生命值最低的 2 个单位各回复 30 ──────────

void SupportEnemy::useSkill(Board& board, std::vector<Unit*>& allUnits)
{
    (void)board;
    std::vector<Unit*> sorted = allUnits;
    std::sort(sorted.begin(), sorted.end(), [](Unit* a, Unit* b) {
        return a->getHp() < b->getHp();
    });
    int healed = 0;
    for (Unit* u : sorted) {
        if (u->isDead() || u->isDisappeared()) continue;
        u->heal(30);
        if (++healed >= 2) break;
    }
}
