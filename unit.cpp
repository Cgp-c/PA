#include "unit.h"

Unit::Unit(const std::string& name, int hp, int maxHp, int x, int y, UnitType type,
           int moveSpeed, int attackSpeed, int startMana,
           int maxMana, int maxMana2)
    : m_name(name)
    , m_hp(hp)
    , m_maxHp(maxHp)
    , m_pos(x, y)
    , m_equipment{nullptr}
    , m_disappeared(false)
    , m_type(type)
    , m_starLevel(0)
    , m_mana(startMana)
    , m_maxMana(maxMana)
    , m_mana2(0)
    , m_maxMana2(maxMana2)
    , m_burning(false)
    , m_burningTurns(0)
    , m_moveSpeed(moveSpeed)
    , m_attackSpeed(attackSpeed)
    , m_moveTimer(0)
    , m_attackTimer(0)
{
}

int Unit::attack(Unit& target)
{
    int damage = getAttackDamage() + getEquipBonusDamage();
    int beforeHp = target.getHp();
    target.takeDamage(damage);
    int dealt = beforeHp - target.getHp();
    if (target.isDead())
        target.setDisappeared(true);
    return dealt;
}

bool Unit::isDead() const { return m_hp <= 0; }
bool Unit::isDisappeared() const { return m_disappeared; }

void Unit::takeDamage(int damage)
{
    m_hp -= damage;
    if (m_hp < 0) m_hp = 0;
}

void Unit::setDisappeared(bool disappeared) { m_disappeared = disappeared; }

int Unit::heal(int amount)
{
    int beforeHp = m_hp;
    int effectiveMax = getMaxHp();
    m_hp += amount;
    if (m_hp > effectiveMax) m_hp = effectiveMax;
    return m_hp - beforeHp;
}

// ─── 法力值 ──────────────────────────────────────────────────

int Unit::getMana() const { return m_mana; }
void Unit::setMana(int mana) { m_mana = std::min(mana, getMaxMana()); }

int Unit::getMaxMana() const
{
    int base = m_maxMana + m_bondManaMod;
    double mult = getEquipManaCapMultiplier();
    return static_cast<int>(base * mult);
}
void Unit::gainMana()
{
    int cap = getMaxMana();
    m_mana = std::min(m_mana + MANA_PER_POINT, cap);
}
void Unit::resetMana() { m_mana = 0; }

// ─── 第二法力值（Boss 进阶技能）─────────────────────────────

int Unit::getMana2() const { return m_mana2; }
int Unit::getMaxMana2() const { return m_maxMana2; }
void Unit::gainMana2()
{
    if (m_mana2 < m_maxMana2) ++m_mana2;
}
void Unit::resetMana2() { m_mana2 = 0; }

void Unit::useSkill2(Board& board, std::vector<Unit*>& allUnits)
{
    (void)board; (void)allUnits;
}

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
int Unit::getMaxHp() const { return static_cast<int>((m_maxHp + getEquipBonusHp()) * m_bondHpMult); }
Position Unit::getPosition() const { return m_pos; }
UnitType Unit::getType() const { return m_type; }
int Unit::getStarLevel() const { return m_starLevel; }
void Unit::setStarLevel(int level) { m_starLevel = level; }

void Unit::setPosition(int x, int y) { m_pos = Position(x, y); }
void Unit::setHp(int hp) { m_hp = hp; }
void Unit::setMaxHp(int maxHp) { m_maxHp = maxHp; }

// ─── 装备系统 ──────────────────────────────────────────────────

bool Unit::equip(Weapon* weapon)
{
    if (!weapon) return false;
    int idx = static_cast<int>(weapon->getEquipType());
    if (m_equipment[idx]) return false; // 已有同类装备
    if (getEquippedCount() >= getMaxEquipSlots()) return false; // 装备位已满
    m_equipment[idx] = weapon;
    // 防御装加成生命值（考虑羁绊倍率）
    if (weapon->getEquipType() == EquipType::Defense)
        m_hp += static_cast<int>(weapon->getBonusHp() * m_bondHpMult);
    return true;
}

void Unit::unequip(EquipType type)
{
    int idx = static_cast<int>(type);
    if (!m_equipment[idx]) return;
    // 卸下防御装时扣除加成生命
    if (type == EquipType::Defense) {
        m_hp -= static_cast<int>(m_equipment[idx]->getBonusHp() * m_bondHpMult);
        if (m_hp < 1) m_hp = 1;
    }
    m_equipment[idx] = nullptr;
}

Weapon* Unit::getEquip(EquipType type) const
{
    return m_equipment[static_cast<int>(type)];
}

int Unit::getMaxEquipSlots() const
{
    return m_starLevel / 2 + 1;
}

int Unit::getEquippedCount() const
{
    int count = 0;
    for (int i = 0; i < static_cast<int>(EquipType::COUNT); ++i)
        if (m_equipment[i]) ++count;
    return count;
}

int Unit::getEquipBonusDamage() const
{
    auto* w = m_equipment[static_cast<int>(EquipType::Attack)];
    return w ? w->getBonusDamage() : 0;
}

int Unit::getEquipBonusHp() const
{
    auto* w = m_equipment[static_cast<int>(EquipType::Defense)];
    return w ? w->getBonusHp() : 0;
}

double Unit::getEquipSpeedMultiplier() const
{
    auto* w = m_equipment[static_cast<int>(EquipType::Speed)];
    return w ? w->getSpeedMultiplier() : 1.0;
}

double Unit::getEquipManaCapMultiplier() const
{
    auto* w = m_equipment[static_cast<int>(EquipType::Mana)];
    return w ? w->getManaCapMultiplier() : 1.0;
}

int Unit::getEquipBonusRange() const
{
    auto* w = m_equipment[static_cast<int>(EquipType::Range)];
    return w ? w->getBonusRange() : 0;
}

int Unit::getMoveSpeed() const { return m_moveSpeed; }

int Unit::getAttackSpeed() const
{
    return static_cast<int>(m_attackSpeed * getEquipSpeedMultiplier());
}

int Unit::getMoveTimer() const { return m_moveTimer; }
int Unit::getAttackTimer() const { return m_attackTimer; }

void Unit::incrementTimers()
{
    ++m_moveTimer;
    ++m_attackTimer;
}

void Unit::resetMoveTimer() { m_moveTimer = 0; }
void Unit::resetAttackTimer() { m_attackTimer = 0; }

// ─── 羁绊效果 ──────────────────────────────────────────────────

void Unit::resetBondEffects()
{
    if (m_bondHpMult != 1.0) {
        int oldMax = static_cast<int>((m_maxHp + getEquipBonusHp()) * m_bondHpMult);
        m_bondHpMult = 1.0;
        int newMax = m_maxHp + getEquipBonusHp();
        m_hp = std::min(m_hp * newMax / std::max(1, oldMax), newMax);
    }
    m_bondHealMult = 1.0;
    m_bondRangeBonus = 0;
    m_bondManaMod = 0;
    m_bondAtkBonus = 0;
}

void Unit::applyBondHpMult(double mult)
{
    int oldMax = static_cast<int>((m_maxHp + getEquipBonusHp()) * m_bondHpMult);
    m_bondHpMult *= mult;
    int newMax = static_cast<int>((m_maxHp + getEquipBonusHp()) * m_bondHpMult);
    m_hp = std::min(m_hp * newMax / std::max(1, oldMax), newMax);
}

void Unit::applyBondHealMult(double mult) { m_bondHealMult *= mult; }
void Unit::applyBondRangeBonus(int bonus) { m_bondRangeBonus += bonus; }
void Unit::applyBondManaMod(int mod) { m_bondManaMod += mod; }
void Unit::applyBondAtkBonus(int bonus) { m_bondAtkBonus += bonus; }

void Unit::revertBondHpMult(double mult)
{
    int oldMax = static_cast<int>((m_maxHp + getEquipBonusHp()) * m_bondHpMult);
    m_bondHpMult /= mult;
    int newMax = static_cast<int>((m_maxHp + getEquipBonusHp()) * m_bondHpMult);
    m_hp = std::min(m_hp * newMax / std::max(1, oldMax), newMax);
}

void Unit::revertBondHealMult(double mult) { m_bondHealMult /= mult; }
void Unit::revertBondRangeBonus(int bonus) { m_bondRangeBonus -= bonus; }
void Unit::revertBondManaMod(int mod) { m_bondManaMod -= mod; }
void Unit::revertBondAtkBonus(int bonus) { m_bondAtkBonus -= bonus; }
