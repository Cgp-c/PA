#include "enemy.h"
#include "hero.h"
#include "board.h"
#include <algorithm>
#include <cstdlib>

Enemy::Enemy(const std::string& name, int hp, int maxHp, int x, int y, UnitType type,
             int moveSpeed, int attackSpeed, int startMana,
             int maxMana, int maxMana2)
    : Unit(name, hp, maxHp, x, y, type, moveSpeed, attackSpeed, startMana, maxMana, maxMana2)
{
}

WarriorEnemy::WarriorEnemy(int starLevel, int x, int y)
    : Enemy("E-Warrior", 100 * (starLevel + 1), 100 * (starLevel + 1),
            x, y, UnitType::Warrior, 30, 60)
{
    m_starLevel = starLevel;
}

MageEnemy::MageEnemy(int starLevel, int x, int y)
    : Enemy("E-Mage", 50 * (starLevel + 1), 50 * (starLevel + 1),
            x, y, UnitType::Mage, 30, 60)
{
    m_starLevel = starLevel;
}

SupportEnemy::SupportEnemy(int starLevel, int x, int y)
    : Enemy("E-Support", 80 * (starLevel + 1), 80 * (starLevel + 1),
            x, y, UnitType::Support, 30, 60)
{
    m_starLevel = starLevel;
}

AssassinEnemy::AssassinEnemy(int starLevel, int x, int y)
    : Enemy("E-Assassin", 40 * (starLevel + 1), 40 * (starLevel + 1),
            x, y, UnitType::Assassin, 30, 40, Unit::MAX_MANA)
{
    m_starLevel = starLevel;
}

BossEnemy::BossEnemy(int x, int y)
    : Enemy("E-Boss", 500, 500, x, y, UnitType::Boss, 60, 60, 0, 3, 5)
{
}

// ─── 战士技能：对最近英雄造成伤害 ──────────────────────

void WarriorEnemy::useSkill(Board& board, std::vector<Unit*>& allUnits)
{
    int dmg = static_cast<int>(80 * (1 + m_starLevel * 0.5));
    Unit* best = nullptr;
    int bestDist = 999;
    for (Unit* u : allUnits) {
        if (u == this || u->isDead() || u->isDisappeared()) continue;
        if (dynamic_cast<Hero*>(u) == nullptr) continue;
        int d = manhattanDist(m_pos, u->getPosition());
        if (d < bestDist) { bestDist = d; best = u; }
    }
    if (best) {
        best->takeDamage(dmg);
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

// ─── 辅助技能：全场生命值最低的 2 个单位各回复 ──────────

void SupportEnemy::useSkill(Board& board, std::vector<Unit*>& allUnits)
{
    (void)board;
    int healAmt = static_cast<int>(30 * (1 + m_starLevel * 0.5));
    std::vector<Unit*> sorted = allUnits;
    std::sort(sorted.begin(), sorted.end(), [](Unit* a, Unit* b) {
        return a->getHp() < b->getHp();
    });
    int healed = 0;
    for (Unit* u : sorted) {
        if (u->isDead() || u->isDisappeared()) continue;
        u->heal(healAmt);
        if (++healed >= 2) break;
    }
}

// ─── 刺客技能：瞬移到最近英雄（曼哈顿距离≤2）的正下方，造成伤害 ──

void AssassinEnemy::useSkill(Board& board, std::vector<Unit*>& allUnits)
{
    int dmg = static_cast<int>(80 * (1 + m_starLevel * 0.5));
    Unit* best = nullptr;
    int bestDist = 999;
    for (Unit* u : allUnits) {
        if (u == this || u->isDead() || u->isDisappeared()) continue;
        if (dynamic_cast<Hero*>(u) == nullptr) continue;
        int d = manhattanDist(m_pos, u->getPosition());
        if (d <= 2 && d < bestDist) { bestDist = d; best = u; }
    }
    if (!best) return;

    Position tp = best->getPosition();

    struct Candidate { int x, y, dist; };
    std::vector<Candidate> cand;
    const int dx[] = {0, 0, -1, 1};
    const int dy[] = {1, -1, 0, 0};

    for (int i = 0; i < 4; ++i) {
        int nx = tp.x + dx[i];
        int ny = tp.y + dy[i];
        if (!board.isValidPosition(nx, ny) || board.isOccupied(nx, ny)) continue;
        cand.push_back({nx, ny, manhattanDist(m_pos, Position(nx, ny))});
    }

    if (cand.empty()) return;

    std::sort(cand.begin(), cand.end(), [](const Candidate& a, const Candidate& b) {
        if (a.dist != b.dist) return a.dist < b.dist;
        return a.y < b.y;
    });

    auto belowIt = std::find_if(cand.begin(), cand.end(), [&](const Candidate& c) {
        return c.x == tp.x && c.y == tp.y + 1;
    });
    Candidate chosen = (belowIt != cand.end()) ? *belowIt : cand.front();

    board.removeUnit(m_pos.x, m_pos.y);
    board.placeUnit(this, chosen.x, chosen.y);

    best->takeDamage(dmg);
    if (best->isDead())
        board.removeUnit(best->getPosition().x, best->getPosition().y);
}

// ─── Boss 基础技能（3 点法力）：攻击全体英雄，各造成 10 伤害 ──

void BossEnemy::useSkill(Board& board, std::vector<Unit*>& allUnits)
{
    (void)board;
    for (Unit* u : allUnits) {
        if (u == this || u->isDead() || u->isDisappeared()) continue;
        if (dynamic_cast<Hero*>(u) == nullptr) continue;
        u->takeDamage(10);
    }
}

// ─── Boss 进阶技能（5 点法力）：攻击米字型四列路径上的全体英雄，各造成 30 伤害 ──

void BossEnemy::useSkill2(Board& board, std::vector<Unit*>& allUnits)
{
    (void)board;
    for (Unit* u : allUnits) {
        if (u == this || u->isDead() || u->isDisappeared()) continue;
        if (dynamic_cast<Hero*>(u) == nullptr) continue;
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
