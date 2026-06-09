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
    virtual double getManaCapMultiplier() const { return 1.0; }
    // ─── 高级装备效果 ──────────────────────────────────────
    virtual int getBonusHeal() const { return 0; }
    virtual int getBonusMana() const { return 0; }           // flat mana cap change
    virtual double getThornsReflect() const { return 0.0; }   // 反伤比例
    virtual int getDefense() const { return 0; }              // 伤害减免
    virtual double getHealMultiplier() const { return 1.0; }  // 受治疗倍率
    virtual double getManaRegenMultiplier() const { return 1.0; } // 法力回复倍率
    virtual bool hasRevive() const { return false; }          // 复活石
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
    std::string getDisplayName() const override { return "\351\255\224\346\263\225\347\237\263"; } // 魔法石
    double getManaCapMultiplier() const override { return 0.5; }
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

// ═══════════════════════════════════════════════════════════════
// 高级装备 (Advanced Weapons) — 由基础装备合成
// ═══════════════════════════════════════════════════════════════

// ─── 高级攻击装 ──────────────────────────────────────────

class RuneGreatsword : public AttackWeapon {
public:
    std::string getName() const override { return "Rune Greatsword"; }
    std::string getDisplayName() const override { return "\347\254\246\346\226\207\345\244\247\345\211\221"; } // 符文大剑
    int getBonusDamage() const override { return 50; }
    int getBonusHeal() const override { return 50; }
    int getBonusMana() const override { return -20; }
};


class SwiftBlade : public AttackWeapon {
public:
    std::string getName() const override { return "Swift Blade"; }
    std::string getDisplayName() const override { return "\346\236\201\351\200\237\346\210\230\345\210\200"; } // 极速战刀
    int getBonusDamage() const override { return 30; }
    double getSpeedMultiplier() const override { return 0.5; }
};

// ─── 高级防御装 ──────────────────────────────────────────

class ThornsArmor : public DefenseWeapon {
public:
    std::string getName() const override { return "Thorns Armor"; }
    std::string getDisplayName() const override { return "\345\217\215\344\274\244\351\223\240\347\224\262"; } // 反伤铠甲
    int getBonusHp() const override { return 200; }
    double getThornsReflect() const override { return 0.5; }
};

class VitalityArmor : public DefenseWeapon {
public:
    std::string getName() const override { return "Vitality Armor"; }
    std::string getDisplayName() const override { return "\347\224\237\345\221\275\351\207\215\347\224\262"; } // 生命重甲
    int getBonusHp() const override { return 400; }
    int getDefense() const override { return 20; }
    double getHealMultiplier() const override { return 1.5; }
};

// ─── 高级攻速装 ──────────────────────────────────────────

class GaleGloves : public SpeedWeapon {
public:
    std::string getName() const override { return "Gale Gloves"; }
    std::string getDisplayName() const override { return "\347\226\276\351\243\216\346\211\213\345\245\227"; } // 疾风手套
    double getSpeedMultiplier() const override { return 0.7; }
    int getBonusMana() const override { return 20; }
};

// ─── 高级蓝装 ────────────────────────────────────────────

class ReviveStone : public ManaWeapon {
public:
    std::string getName() const override { return "Revive Stone"; }
    std::string getDisplayName() const override { return "\345\244\215\346\264\273\347\237\263"; } // 复活石
    int getBonusHp() const override { return 100; }
    double getManaRegenMultiplier() const override { return 2.0; }
    double getManaCapMultiplier() const override { return 1.0; }
    bool hasRevive() const override { return true; }
};

// ─── 高级攻击距离装 ──────────────────────────────────────

class SniperCrossbow : public RangeWeapon {
public:
    std::string getName() const override { return "Sniper Crossbow"; }
    std::string getDisplayName() const override { return "\347\213\231\345\207\273\345\274\251"; } // 狙击弩
    int getBonusDamage() const override { return 40; }
    int getBonusRange() const override { return 2; }
};

#endif // WEAPON_H