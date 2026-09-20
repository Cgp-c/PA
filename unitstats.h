#ifndef UNITSTATS_H
#define UNITSTATS_H

// ═══════════════════════════════════════════════════════════════
// 单位数值表（数据驱动）
//
// 平衡数值集中在 config/unitstats.json，改表不用重编译；
// 文件缺失/字段缺失/损坏时逐字段回退到编译期内置默认值，
// 游戏永远不会因为配置问题而崩溃。
//
// 英雄侧与敌方侧共用同一份表 —— 根除历史上"两份硬编码漂移"的问题。
// ═══════════════════════════════════════════════════════════════

#include "unit.h"
#include <QString>

struct UnitStats {
    int hp = 100;             // 0 星基础生命（实际 = hp × (整星 + 1)）
    int atk = 20;             // 0 星基础攻击（0 = 无攻击/治疗型）
    int heal = 0;             // 每次治疗量（0 = 非治疗型）
    int range = 1;            // 攻击/治疗射程
    int moveSpeed = 60;       // 移动间隔（帧，越小越快）
    int attackSpeed = 120;    // 攻击间隔（帧）
    int skillDmg = 80;        // 技能基础数值（伤害/治疗/DOT 强度，随星级成长）
    int cost = 100;           // 招募价格（0 = 不可招募）
    int sellPrice = 100;      // 卖出价
    int goldValue = 80;       // 敌方击杀金币
    QString name = "Warrior"; // 名称（英雄前缀 A- / 敌方前缀 E-）
};

// 终极角色专用：生命/攻击由三个素材三星基础值求和 × 比例动态生成
struct UltimateStats {
    double hpRatio = 0.8;    // HP = hpRatio × Σ(素材三星基础HP)
    double atkRatio = 0.8;   // ATK = atkRatio × Σ(素材三星基础ATK)
    int defaultHp = 900;     // 无合成上下文时的默认档（自定义难度直选）
    int defaultAtk = 220;
    int range = 2;
    int moveSpeed = 60;
    int attackSpeed = 110;
    int skillDmg = 120;      // 天罚：全体敌方伤害
    int sellPrice = 500;
};

// 指定职业的数值（全局单例，首次调用时加载 JSON）
const UnitStats& statsOf(UnitType t);
const UltimateStats& ultimateStats();

// 按星级的实际基础值（不含装备/羁绊加成；终极合成公式与显示用）
inline int baseHpAtStar(UnitType t, int starLevel)
{
    return statsOf(t).hp * (starLevel / 2 + 1);
}
inline int baseAtkAtStar(UnitType t, int starLevel)
{
    return statsOf(t).atk * (starLevel / 2 + 1);
}

// 可招募职业池（招募/关卡/无尽演化共用；Boss 与终极角色不在池内）
inline constexpr UnitType RECRUITABLE_TYPES[] = {
    UnitType::Warrior, UnitType::Mage, UnitType::Support, UnitType::Assassin,
    UnitType::Hunter, UnitType::Knight, UnitType::Shaman,
};
inline constexpr int RECRUITABLE_COUNT = 7;

inline bool isRecruitableType(UnitType t)
{
    for (UnitType r : RECRUITABLE_TYPES)
        if (r == t) return true;
    return false;
}

// 无星级概念的类型（星级框置灰/锁定）
inline bool isStarlessType(UnitType t)
{
    return t == UnitType::Boss || t == UnitType::Ultimate;
}

#endif // UNITSTATS_H
