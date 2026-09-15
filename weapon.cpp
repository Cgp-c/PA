#include "weapon.h"

Weapon* Weapon::create(const std::string& name)
{
    if (name == "Iron Sword")      return new BasicAttackWeapon;
    if (name == "Chain Mail")      return new BasicDefenseWeapon;
    if (name == "Speed Gloves")    return new BasicSpeedWeapon;
    if (name == "Blue Crystal")    return new BasicManaWeapon;
    if (name == "Warhorse")        return new BasicRangeWeapon;
    if (name == "Rune Greatsword") return new RuneGreatsword;
    if (name == "Swift Blade")     return new SwiftBlade;
    if (name == "Thorns Armor")    return new ThornsArmor;
    if (name == "Vitality Armor")  return new VitalityArmor;
    if (name == "Gale Gloves")     return new GaleGloves;
    if (name == "Revive Stone")    return new ReviveStone;
    if (name == "Sniper Crossbow") return new SniperCrossbow;
    return nullptr;
}
