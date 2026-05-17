#include "unit.h"

Unit::Unit(const std::string& name, int hp, int maxHp, int x, int y, UnitType type)
    : m_name(name)
    , m_hp(hp)
    , m_maxHp(maxHp)
    , m_pos(x, y)
    , m_equipment(nullptr)
    , m_disappeared(false)
    , m_type(type)
    , m_mana(0)
    , m_burning(false)
    , m_burningTurns(0)
{
}

void Unit::attack(Unit& target)
{
    int damage = getAttackDamage();
    if (m_equipment)
        damage += m_equipment->getDamage();
    target.takeDamage(damage);
    if (target.isDead())
        target.setDisappeared(true);
}

bool Unit::isDead() const { return m_hp <= 0; }
bool Unit::isDisappeared() const { return m_disappeared; }

void Unit::takeDamage(int damage)
{
    m_hp -= damage;
    if (m_hp < 0) m_hp = 0;
}

void Unit::setDisappeared(bool disappeared) { m_disappeared = disappeared; }

void Unit::heal(int amount)
{
    m_hp += amount;
    if (m_hp > m_maxHp) m_hp = m_maxHp;
}

// ─── 法力值 ──────────────────────────────────────────────────

int Unit::getMana() const { return m_mana; }
int Unit::getMaxMana() const { return MAX_MANA; }
void Unit::gainMana()
{
    if (m_mana < MAX_MANA) ++m_mana;
}
void Unit::resetMana() { m_mana = 0; }

// ─── 燃烧 ────────────────────────────────────────────────────

bool Unit::isBurning() const { return m_burning; }
int Unit::getBurningTurns() const { return m_burningTurns; }

void Unit::applyBurning(int turns)
{
    m_burning = true;
    m_burningTurns = turns;
}

void Unit::tickBurning()
{
    if (!m_burning) return;
    takeDamage(10);
    --m_burningTurns;
    if (m_burningTurns <= 0)
        m_burning = false;
}

// ─── getter / setter ─────────────────────────────────────────

std::string Unit::getName() const { return m_name; }
int Unit::getHp() const { return m_hp; }
int Unit::getMaxHp() const { return m_maxHp; }
Position Unit::getPosition() const { return m_pos; }
Weapon* Unit::getEquipment() const { return m_equipment; }
UnitType Unit::getType() const { return m_type; }

void Unit::setPosition(int x, int y) { m_pos = Position(x, y); }
void Unit::setEquipment(Weapon* weapon) { m_equipment = weapon; }
void Unit::setHp(int hp) { m_hp = hp; }
void Unit::setMaxHp(int maxHp) { m_maxHp = maxHp; }
