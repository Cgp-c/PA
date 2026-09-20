#ifndef UNIT_H
#define UNIT_H

#include <string>
#include <vector>
#include "weapon.h"

struct Position {
    int x;
    int y;
    Position(int x = 0, int y = 0) : x(x), y(y) {}
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

enum class UnitType {
    Warrior = 0, Mage = 1, Support = 2, Assassin = 3,
    Boss = 4,          // ← 值固定：存档/联机按整数存类型
    Hunter = 5,        // 射手：远程物理
    Knight = 6,        // 骑士：高血坦克
    Shaman = 7,        // 萨满：持续毒伤
    Ultimate = 8,      // 终极角色（三职业三星合成，动态数值）
    COUNT = 9
};

inline int manhattanDist(const Position& a, const Position& b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

class Board;

class Unit {
public:
    static constexpr int BASE_MAX_MANA = 100;
    static constexpr int MANA_PER_POINT = 20;

    Unit(const std::string& name, int hp, int maxHp, int x, int y, UnitType type,
         int moveSpeed = 30, int attackSpeed = 60, int startMana = 0,
         int maxMana = BASE_MAX_MANA, int maxMana2 = 0);
    virtual ~Unit() = default;

    virtual int attack(Unit& target);

    bool isDead() const;
    bool isDisappeared() const;

    void takeDamage(int damage);
    void setDisappeared(bool disappeared);
    int heal(int amount);

    // 法力值
    int getMana() const;
    int getMaxMana() const;
    void setMana(int mana);
    void gainMana();
    void resetMana();

    // 第二法力值（Boss 进阶技能）
    int getMana2() const;
    int getMaxMana2() const;
    void gainMana2();
    void resetMana2();

    // 技能（纯虚，子类各自实现）
    virtual void useSkill(Board& board, std::vector<Unit*>& allUnits) = 0;
    virtual void useSkill2(Board& board, std::vector<Unit*>& allUnits);

    // 新职业技能的通用实现（Hero/Enemy 共用，unit.cpp）
    void hunterSkill(Board& board, std::vector<Unit*>& allUnits);    // 三连箭
    void knightSkill(Board& board, std::vector<Unit*>& allUnits);    // 盾击+自疗
    void shamanSkill(Board& board, std::vector<Unit*>& allUnits);    // 腐蚀之种
    void ultimateSkill(Board& board, std::vector<Unit*>& allUnits);  // 天罚

    // 燃烧状态（萨满的毒复用同一 DOT 管线，green 标记控制渲染颜色）
    bool isBurning() const;
    int getBurningTurns() const;
    bool isGreenDot() const { return m_poisonDot.active; }
    void applyBurning(int turns, int damage = 10, bool green = false);
    void tickBurning();

    std::string getName() const;
    int getHp() const;
    int getMaxHp() const;
    int getBaseMaxHp() const { return m_maxHp; }   // 不含装备/羁绊的基础值（序列化用）
    Position getPosition() const;
    UnitType getType() const;

    virtual int getAttackRange() const = 0;
    virtual int getAttackDamage() const = 0;
    virtual int getHealAmount() const { return 0; }
    virtual bool canHeal() const { return false; }

    int getStarLevel() const;
    void setStarLevel(int level);

    void setPosition(int x, int y);
    void setHp(int hp);
    void setMaxHp(int maxHp);   // 联机阵容恢复等场景需要显式重设上限

    // 装备系统
    bool equip(Weapon* weapon);
    void unequip(EquipType type);
    Weapon* getEquip(EquipType type) const;
    int getMaxEquipSlots() const;
    int getEquippedCount() const;

    int getEquipBonusDamage() const;
    int getEquipBonusHp() const;
    double getEquipSpeedMultiplier() const;
    double getEquipManaCapMultiplier() const;
    int getEquipBonusRange() const;
    int getEquipBonusHeal() const;
    int getEquipBonusMana() const;
    double getEquipThornsReflect() const;
    int getEquipDefense() const;
    double getEquipHealMultiplier() const;
    double getEquipManaRegenMultiplier() const;
    bool hasEquipRevive() const;

    // 速度 / 计时器
    int getMoveSpeed() const;
    int getAttackSpeed() const;
    int getMoveTimer() const;
    int getAttackTimer() const;
    void incrementTimers();
    void resetMoveTimer();
    void resetAttackTimer();

    // 羁绊效果
    void resetBondEffects();
    void applyBondHpMult(double mult);
    void applyBondHealMult(double mult);
    void applyBondRangeBonus(int bonus);
    void applyBondManaMod(int mod);
    void applyBondAtkBonus(int bonus);
    void revertBondHpMult(double mult);
    void revertBondHealMult(double mult);
    void revertBondRangeBonus(int bonus);
    void revertBondManaMod(int mod);
    void revertBondAtkBonus(int bonus);

    double getBondHpMult() const { return m_bondHpMult; }
    double getBondHealMult() const { return m_bondHealMult; }
    int getBondRangeBonus() const { return m_bondRangeBonus; }
    int getBondManaMod() const { return m_bondManaMod; }
    int getBondAtkBonus() const { return m_bondAtkBonus; }

    void setClone(bool c) { m_isClone = c; }
    bool isClone() const { return m_isClone; }

    // 复活石
    bool hasReviveTriggered() const { return m_reviveTriggered; }
    void clearReviveTriggered() { m_reviveTriggered = false; }

    // 战斗统计（每次开战重置，战后统计面板读取）
    void resetBattleStats();
    void addStatDealt(int v)   { m_statDealt += v; }
    void addStatHealed(int v)  { m_statHealed += v; }
    void addStatKill()         { ++m_statKills; }
    int getStatDealt() const   { return m_statDealt; }
    int getStatTaken() const   { return m_statTaken; }
    int getStatHealed() const  { return m_statHealed; }
    int getStatKills() const   { return m_statKills; }

protected:
    // 四职业技能的通用实现，Hero/Enemy 子类的 useSkill 一行转发即可，
    // 敌我判定由 isHeroSide() 参数化，数值随 m_starLevel 缩放
    void warriorSkill(Board& board, std::vector<Unit*>& allUnits);   // 对最近敌方造成技能伤害
    void mageSkill(Board& board, std::vector<Unit*>& allUnits);      // 周围 5×5 敌方燃烧
    void supportSkill(Board& board, std::vector<Unit*>& allUnits);   // 全场血量最低 2 个单位治疗
    void assassinSkill(Board& board, std::vector<Unit*>& allUnits);  // 瞬移至最近敌方旁并造成伤害

    // 敌我阵营判定（Hero 方 = true），索敌与技能共用
    virtual bool isHeroSide() const = 0;

public:
    bool isOpponentOf(const Unit* other) const { return isHeroSide() != other->isHeroSide(); }

    static constexpr int SKILL_DMG = 80;       // 战士/刺客技能伤害基数
    static constexpr int SKILL_HEAL = 30;      // 辅助技能治疗基数
    static constexpr int MAGE_BURN_BASE = 10;  // 法师燃烧基础伤害

protected:
    std::string m_name;
    int m_hp;
    int m_maxHp;
    Position m_pos;
    Weapon* m_equipment[static_cast<int>(EquipType::COUNT)];
    bool m_disappeared;
    UnitType m_type;
    int m_starLevel;
    int m_mana;
    int m_maxMana;
    int m_mana2;
    int m_maxMana2;
    // DOT 双槽：火焰燃烧（红）与毒素（绿）可共存
    // 共存时每 tick 伤害 = (火焰 + 毒素) × 1.2（特殊技能合体效果）
    struct DotState {
        bool active = false;
        int turns = 0;
        int damage = 0;
    };
    DotState m_fireDot;
    DotState m_poisonDot;
    int m_moveSpeed;
    int m_attackSpeed;
    int m_moveTimer;
    int m_attackTimer;

    // 羁绊效果
    double m_bondHpMult = 1.0;
    double m_bondHealMult = 1.0;
    int m_bondRangeBonus = 0;
    int m_bondManaMod = 0;
    int m_bondAtkBonus = 0;
    bool m_isClone = false;
    bool m_reviveTriggered = false;

    // 战斗统计
    int m_statDealt = 0;
    int m_statTaken = 0;
    int m_statHealed = 0;
    int m_statKills = 0;
};

#endif // UNIT_H
