#ifndef UNITVISUALS_H
#define UNITVISUALS_H

// 单位视觉共享定义 —— 主窗口与各独立窗口（自定义难度等）共用的
// 职业配色与单字标签，保证两处渲染风格一致。

#include <QColor>
#include <QString>
#include "unit.h"

// 职业主色（isHero 区分玩家/敌方同职业的明暗）
inline QColor typeFillColor(UnitType t, bool isHero)
{
    switch (t) {
        case UnitType::Warrior:  return isHero ? QColor(210, 100, 30)  : QColor(180, 70, 20);
        case UnitType::Mage:     return isHero ? QColor(130, 80, 210)  : QColor(100, 55, 170);
        case UnitType::Support:  return isHero ? QColor(55, 170, 100)  : QColor(40, 140, 70);
        case UnitType::Assassin: return isHero ? QColor(200, 180, 40)  : QColor(160, 140, 20);
        case UnitType::Boss:     return isHero ? QColor(150, 40, 90)   : QColor(120, 25, 70);
    }
    return QColor(128, 128, 128);
}

// 单字中文职业标签
inline QString typeLabel(UnitType t)
{
    switch (t) {
        case UnitType::Warrior:  return QString::fromUtf8("战");
        case UnitType::Mage:     return QString::fromUtf8("法");
        case UnitType::Support:  return QString::fromUtf8("辅");
        case UnitType::Assassin: return QString::fromUtf8("刺");
        case UnitType::Boss:     return QString::fromUtf8("王");
    }
    return QString("?");
}

// 职业英文全名（信息面板/配置界面用）
inline const char* unitTypeNameEn(UnitType t)
{
    switch (t) {
        case UnitType::Warrior:  return "Warrior";
        case UnitType::Mage:     return "Mage";
        case UnitType::Support:  return "Support";
        case UnitType::Assassin: return "Assassin";
        case UnitType::Boss:     return "Boss";
    }
    return "?";
}

#endif // UNITVISUALS_H
