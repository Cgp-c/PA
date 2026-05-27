#ifndef WEAPON_H
#define WEAPON_H

#include <string>

enum class EquipType { Attack, Defense, Speed, Mana, Range, COUNT };

class Weapon {
public:
    virtual ~Weapon() = default;
    virtual std::string getName() const = 0;
    virtual std::string getDisplayName() const = 0;
    virtual EquipType getEquipType() const = 0;
    virtual int getBonusDamage() const { return 0; }
    virtual int getBonusHp() const { return 0; }
    virtual double getSpeedMultiplier() const { return 1.0; }
    virtual int getSkillManaCost() const { return 0; }
    virtual int getBonusRange() const { return 0; }
};

// ─── 攻击装 ──────────────────────────────────────────────

class AttackWeapon : public Weapon {
public:
    EquipType getEquipType() const final { return EquipType::Attack; }
};

class BasicAttackWeapon : public AttackWeapon {
public:
    std::string getName() const override { return "Iron Sword"; }
    std::string getDisplayName() const override { return "\345\211\221"; } // 剑
    int getBonusDamage() const override { return 30; }
};

// ─── 防御装 ──────────────────────────────────────────────

class DefenseWeapon : public Weapon {
public:
    EquipType getEquipType() const final { return EquipType::Defense; }
};

class BasicDefenseWeapon : public DefenseWeapon {
public:
    std::string getName() const override { return "Chain Mail"; }
    std::string getDisplayName() const override { return "\347\224\262"; } // 甲
    int getBonusHp() const override { return 150; }
};

// ─── 攻速装 ──────────────────────────────────────────────

class SpeedWeapon : public Weapon {
public:
    EquipType getEquipType() const final { return EquipType::Speed; }
};

class BasicSpeedWeapon : public SpeedWeapon {
public:
    std::string getName() const override { return "Speed Gloves"; }
    std::string getDisplayName() const override { return "\346\224\273\351\200\237"; } // 攻速
    double getSpeedMultiplier() const override { return 0.8; }
};

// ─── 蓝装 ────────────────────────────────────────────────

class ManaWeapon : public Weapon {
public:
    EquipType getEquipType() const final { return EquipType::Mana; }
};

class BasicManaWeapon : public ManaWeapon {
public:
    std::string getName() const override { return "Blue Crystal"; }
    std::string getDisplayName() const override { return "\350\223\235\345\256\235\347\237\263"; } // 蓝宝石
    int getSkillManaCost() const override { return 3; }
};

// ─── 攻击距离装 ──────────────────────────────────────────

class RangeWeapon : public Weapon {
public:
    EquipType getEquipType() const final { return EquipType::Range; }
};

class BasicRangeWeapon : public RangeWeapon {
public:
    std::string getName() const override { return "Warhorse"; }
    std::string getDisplayName() const override { return "\346\210\230\351\251\254"; } // 战马
    int getBonusRange() const override { return 1; }
};

#endif // WEAPON_H
