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
    , m_burningDamage(10)
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

    // 反伤：目标受到伤害后反弹给攻击者
    double thorns = target.getEquipThornsReflect();
    if (thorns > 0.0 && dealt > 0) {
        int reflectDmg = static_cast<int>(dealt * thorns);
        if (reflectDmg > 0)
            takeDamage(reflectDmg);
    }

    // 目标复活石触发则不死
    if (target.isDead() && !target.hasReviveTriggered())
        target.setDisappeared(true);
    return dealt;
}

bool Unit::isDead() const { return m_hp <= 0; }
bool Unit::isDisappeared() const { return m_disappeared; }

void Unit::takeDamage(int damage)
{
    // 防御减伤（生命重甲等）
    int defense = getEquipDefense();
    int effectiveDmg = damage - defense;
    if (effectiveDmg < 0) effectiveDmg = 0;
    m_hp -= effectiveDmg;
    if (m_hp <= 0) {
        // 检查复活石
        if (hasEquipRevive()) {
            // 直接移除复活石（跳过 unequip 避免 HP 二次调整）
            for (int i = 0; i < static_cast<int>(EquipType::COUNT); ++i) {
                if (m_equipment[i] && m_equipment[i]->hasRevive()) {
                    m_equipment[i] = nullptr;
                    break;
                }
            }
            // 回满生命值（getMaxHp 不再含复活石的 HP 加成）
            m_hp = getMaxHp();
            m_mana = 0;
            m_mana2 = 0;
            m_reviveTriggered = true;
            return;
        }
        m_hp = 0;
    }
}

void Unit::setDisappeared(bool disappeared) { m_disappeared = disappeared; }

int Unit::heal(int amount)
{
    int beforeHp = m_hp;
    int effectiveMax = getMaxHp();
    // 受治疗倍率（生命重甲等装备效果）
    double healMult = getEquipHealMultiplier();
    int adjustedAmount = static_cast<int>(amount * healMult);
    m_hp += adjustedAmount;
    if (m_hp > effectiveMax) m_hp = effectiveMax;
    return m_hp - beforeHp;
}

// ─── 法力值 ──────────────────────────────────────────────────

int Unit::getMana() const { return m_mana; }
void Unit::setMana(int mana) { m_mana = std::min(mana, getMaxMana()); }

int Unit::getMaxMana() const
{
    int base = m_maxMana + m_bondManaMod + getEquipBonusMana();
    double mult = getEquipManaCapMultiplier();
    int result = static_cast<int>(base * mult);
    if (result < 10) result = 10;
    return result;
}
void Unit::gainMana()
{
    int cap = getMaxMana();
    int regen = static_cast<int>(MANA_PER_POINT * getEquipManaRegenMultiplier());
    m_mana = std::min(m_mana + regen, cap);
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

void Unit::applyBurning(int turns, int damage)
{
    m_burning = true;
    m_burningTurns = turns;
    m_burningDamage = damage;
}

void Unit::tickBurning()
{
    if (!m_burning) return;
    takeDamage(m_burningDamage);
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
    // 加成生命值（防御装或其他带 HP 的装备，考虑羁绊倍率）
    int bonusHp = weapon->getBonusHp();
    if (bonusHp > 0)
        m_hp += static_cast<int>(bonusHp * m_bondHpMult);
    return true;
}

void Unit::unequip(EquipType type)
{
    int idx = static_cast<int>(type);
    if (!m_equipment[idx]) return;
    // 卸下时扣除加成 HP
    int bonusHp = m_equipment[idx]->getBonusHp();
    if (bonusHp > 0) {
        m_hp -= static_cast<int>(bonusHp * m_bondHpMult);
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
    int total = 0;
    for (int i = 0; i < static_cast<int>(EquipType::COUNT); ++i)
        if (m_equipment[i])
            total += m_equipment[i]->getBonusHp();
    return total;
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

int Unit::getEquipBonusHeal() const
{
    auto* w = m_equipment[static_cast<int>(EquipType::Attack)];
    return w ? w->getBonusHeal() : 0;
}

int Unit::getEquipBonusMana() const
{
    int total = 0;
    for (int i = 0; i < static_cast<int>(EquipType::COUNT); ++i)
        if (m_equipment[i])
            total += m_equipment[i]->getBonusMana();
    return total;
}

double Unit::getEquipThornsReflect() const
{
    double total = 0.0;
    for (int i = 0; i < static_cast<int>(EquipType::COUNT); ++i)
        if (m_equipment[i])
            total += m_equipment[i]->getThornsReflect();
    return total;
}

int Unit::getEquipDefense() const
{
    int total = 0;
    for (int i = 0; i < static_cast<int>(EquipType::COUNT); ++i)
        if (m_equipment[i])
            total += m_equipment[i]->getDefense();
    return total;
}

double Unit::getEquipHealMultiplier() const
{
    double total = 1.0;
    for (int i = 0; i < static_cast<int>(EquipType::COUNT); ++i)
        if (m_equipment[i]) {
            double m = m_equipment[i]->getHealMultiplier();
            if (m > 1.0) total *= m;
        }
    return total;
}

double Unit::getEquipManaRegenMultiplier() const
{
    double total = 1.0;
    for (int i = 0; i < static_cast<int>(EquipType::COUNT); ++i)
        if (m_equipment[i]) {
            double m = m_equipment[i]->getManaRegenMultiplier();
            if (m > 1.0) total *= m;
        }
    return total;
}

bool Unit::hasEquipRevive() const
{
    for (int i = 0; i < static_cast<int>(EquipType::COUNT); ++i)
        if (m_equipment[i] && m_equipment[i]->hasRevive())
            return true;
    return false;
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
