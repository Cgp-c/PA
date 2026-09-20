#include "unitstats.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

// ═══════════════════════════════════════════════════════════════
// 内置默认值（config/unitstats.json 缺失/损坏时逐字段回退）
// ═══════════════════════════════════════════════════════════════

namespace {

struct Defaults {
    const char* key;
    UnitStats st;
};

// 与 v0.22 及之前的硬编码数值完全一致（新职业为本次新增定位）
const Defaults DEFAULTS[] = {
    { "warrior", {100, 20,  0, 1, 60, 120, 80, 100, 100, 80,  "Warrior"} },
    { "mage",    { 50, 10,  0, 4, 60, 120, 10,  80,  80, 60,  "Mage"} },
    { "support", { 80,  0, 20, 2, 60, 120, 30,  50,  50, 30,  "Support"} },
    { "assassin",{ 15, 50,  0, 1, 60,  80, 80,  60,  60, 30,  "Assassin"} },
    { "hunter",  { 70, 30,  0, 3, 60, 100, 80,  90,  90, 70,  "Hunter"} },
    { "knight",  {220, 12,  0, 1, 90, 120, 80, 110, 110, 90,  "Knight"} },
    { "shaman",  { 60, 12,  0, 3, 60, 120, 60,  85,  85, 65,  "Shaman"} },
    { "boss",    {500, 20,  0, 1, 120, 120, 30, 0,   0,   200, "Boss"} },
};
constexpr int DEFAULT_COUNT = sizeof(DEFAULTS) / sizeof(DEFAULTS[0]);

UnitStats g_stats[static_cast<int>(UnitType::Ultimate) + 1];
UltimateStats g_ultimate;
bool g_loaded = false;

// JSON 逐字段读取：缺字段回退默认
int getInt(const QJsonObject& o, const char* k, int def)
{
    const auto v = o.value(QLatin1String(k));
    return v.isDouble() ? v.toInt() : def;
}
double getDouble(const QJsonObject& o, const char* k, double def)
{
    const auto v = o.value(QLatin1String(k));
    return v.isDouble() ? v.toDouble() : def;
}

void loadOnce()
{
    g_loaded = true;

    // 1) 内置默认
    for (int i = 0; i < DEFAULT_COUNT; ++i) {
        // DEFAULTS 数组顺序与 UnitType 枚举前 8 个一一对应
        // （warrior..shaman..boss 依次为枚举 0..4 + 新增 5..7 后 boss=4）
        const int idx = [] (int order) -> int {
            // DEFAULTS 顺序: warrior mage support assassin hunter knight shaman boss
            // 枚举顺序:      warrior mage support assassin boss  hunter knight shaman
            switch (order) {
                case 0: return static_cast<int>(UnitType::Warrior);
                case 1: return static_cast<int>(UnitType::Mage);
                case 2: return static_cast<int>(UnitType::Support);
                case 3: return static_cast<int>(UnitType::Assassin);
                case 4: return static_cast<int>(UnitType::Hunter);
                case 5: return static_cast<int>(UnitType::Knight);
                case 6: return static_cast<int>(UnitType::Shaman);
                case 7: return static_cast<int>(UnitType::Boss);
            }
            return 0;
        } (i);
        g_stats[idx] = DEFAULTS[i].st;
    }

    // 2) JSON 覆盖（存在则逐字段覆盖）
    QFile f(QStringLiteral("config/unitstats.json"));
    const QStringList roots = {
        QStringLiteral("config/unitstats.json"),
        QStringLiteral("../config/unitstats.json"),
    };
    QFile file;
    for (const QString& p : roots) {
        file.setFileName(p);
        if (file.open(QIODevice::ReadOnly)) break;
    }
    if (!file.isOpen()) return;   // 无配置文件：全默认

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return;   // 损坏：全默认

    const QJsonObject root = doc.object();
    for (int t = 0; t <= static_cast<int>(UnitType::Ultimate); ++t) {
        const char* key = nullptr;
        switch (static_cast<UnitType>(t)) {
            case UnitType::Warrior:  key = "warrior"; break;
            case UnitType::Mage:     key = "mage"; break;
            case UnitType::Support:  key = "support"; break;
            case UnitType::Assassin: key = "assassin"; break;
            case UnitType::Boss:     key = "boss"; break;
            case UnitType::Hunter:   key = "hunter"; break;
            case UnitType::Knight:   key = "knight"; break;
            case UnitType::Shaman:   key = "shaman"; break;
            case UnitType::Ultimate: key = "ultimate"; break;
        }
        if (!key) continue;
        const QJsonObject o = root.value(QLatin1String(key)).toObject();
        if (o.isEmpty()) continue;

        UnitStats& st = g_stats[t];
        st.hp          = getInt(o, "hp", st.hp);
        st.atk         = getInt(o, "atk", st.atk);
        st.heal        = getInt(o, "heal", st.heal);
        st.range       = getInt(o, "range", st.range);
        st.moveSpeed   = getInt(o, "moveSpeed", st.moveSpeed);
        st.attackSpeed = getInt(o, "attackSpeed", st.attackSpeed);
        st.skillDmg    = getInt(o, "skillDmg", st.skillDmg);
        st.cost        = getInt(o, "cost", st.cost);
        st.sellPrice   = getInt(o, "sellPrice", st.sellPrice);
        st.goldValue   = getInt(o, "goldValue", st.goldValue);
        if (o.contains("name")) st.name = o.value("name").toString();
    }

    // 终极角色专表
    const QJsonObject uo = root.value("ultimate").toObject();
    if (!uo.isEmpty()) {
        g_ultimate.hpRatio     = getDouble(uo, "hpRatio", g_ultimate.hpRatio);
        g_ultimate.atkRatio    = getDouble(uo, "atkRatio", g_ultimate.atkRatio);
        g_ultimate.defaultHp   = getInt(uo, "defaultHp", g_ultimate.defaultHp);
        g_ultimate.defaultAtk  = getInt(uo, "defaultAtk", g_ultimate.defaultAtk);
        g_ultimate.range       = getInt(uo, "range", g_ultimate.range);
        g_ultimate.moveSpeed   = getInt(uo, "moveSpeed", g_ultimate.moveSpeed);
        g_ultimate.attackSpeed = getInt(uo, "attackSpeed", g_ultimate.attackSpeed);
        g_ultimate.skillDmg    = getInt(uo, "skillDmg", g_ultimate.skillDmg);
        g_ultimate.sellPrice   = getInt(uo, "sellPrice", g_ultimate.sellPrice);
    }
}

} // namespace

const UnitStats& statsOf(UnitType t)
{
    if (!g_loaded) loadOnce();
    const int idx = static_cast<int>(t);
    if (idx < 0 || idx > static_cast<int>(UnitType::Ultimate))
        return g_stats[0];   // 防御：非法枚举回退到战士档
    return g_stats[idx];
}

const UltimateStats& ultimateStats()
{
    if (!g_loaded) loadOnce();
    return g_ultimate;
}
