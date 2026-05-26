#include "hero.h"
#include "enemy.h"
#include "board.h"
#include <algorithm>
#include <cstdlib>

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
           x, y, UnitType::Assassin, 60, 80, Unit::MAX_MANA)
{
    m_starLevel = starLevel;
}

// ─── 战士技能：对最近敌人造成伤害 ──────────────────────

void WarriorHero::useSkill(Board& board, std::vector<Unit*>& allUnits)
{
    int dmg = static_cast<int>(BASE_SKILL_DMG * (1 + (m_starLevel / 2) * 0.5));
    Unit* best = nullptr;
    int bestDist = 999;
    for (Unit* u : allUnits) {
        if (u == this || u->isDead() || u->isDisappeared()) continue;
        if (dynamic_cast<Enemy*>(u) == nullptr) continue;
        int d = manhattanDist(m_pos, u->getPosition());
        if (d < bestDist) { bestDist = d; best = u; }
    }
    if (best) {
        best->takeDamage(dmg);
        if (best->isDead())
            board.removeUnit(best->getPosition().x, best->getPosition().y);
    }
}

// ─── 法师技能：周围 5×5 敌方全体燃烧 4 回合 ─────────────────

void MageHero::useSkill(Board& board, std::vector<Unit*>& allUnits)
{
    (void)board;
    for (Unit* u : allUnits) {
        if (u == this || u->isDead() || u->isDisappeared()) continue;
        if (dynamic_cast<Enemy*>(u) == nullptr) continue;
        int dx = std::abs(u->getPosition().x - m_pos.x);
        int dy = std::abs(u->getPosition().y - m_pos.y);
        if (dx <= 2 && dy <= 2) {
            u->applyBurning(4);
        }
    }
}

// ─── 辅助技能：全场生命值最低的 2 个单位各回复 ──────────

void SupportHero::useSkill(Board& board, std::vector<Unit*>& allUnits)
{
    (void)board;
    int healAmt = static_cast<int>(BASE_SKILL_HEAL * (1 + (m_starLevel / 2) * 0.5));
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

// ─── 刺客技能：瞬移到最近敌方（曼哈顿距离≤2）的正下方，造成伤害 ──

void AssassinHero::useSkill(Board& board, std::vector<Unit*>& allUnits)
{
    int dmg = static_cast<int>(BASE_SKILL_DMG * (1 + (m_starLevel / 2) * 0.5));
    Unit* best = nullptr;
    int bestDist = 999;
    for (Unit* u : allUnits) {
        if (u == this || u->isDead() || u->isDisappeared()) continue;
        if (dynamic_cast<Enemy*>(u) == nullptr) continue;
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
