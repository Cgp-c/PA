#ifndef UNITVISUALS_H
#define UNITVISUALS_H

// 单位视觉共享定义 —— 主窗口与各独立窗口（自定义难度等）共用的
// 职业立绘、配色与单字标签，保证各处渲染风格一致。

#include <QColor>
#include <QString>
#include <QPixmap>
#include <QPainter>
#include <QFont>
#include <QCoreApplication>
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

// ── 职业立绘：按 UnitType 惰性加载并全局缓存 ─────────────────────
// 首次调用时从磁盘加载（多路径回退，兼容项目根/build/exe 目录启动），
// 之后直接命中静态缓存；加载失败保持空 QPixmap，由调用方回退色块。
inline const QPixmap& unitPortrait(UnitType t)
{
    static QPixmap cache[static_cast<int>(UnitType::Boss) + 1];
    static bool tried = false;
    if (!tried) {
        tried = true;   // 失败也只尝试一次，避免每帧重复磁盘 IO

        // 立绘映射（奇幻系列，与 Boss 的 WailingPrince 同套素材）：
        //   战士=Pirate(持械近战)  法师=Witch(施法者)
        //   辅助=GreenGoo(治疗绿)  刺客=Bird(敏捷)  Boss=WailingPrince
        static const char* files[] = {
            "World01_007_Pirate.png",        // Warrior
            "World01_006_Witch.png",         // Mage
            "World01_001_GreenGoo.png",      // Support
            "World01_003_Bird.png",          // Assassin
            "World01_004_WailingPrince.png", // Boss
        };
        static_assert(sizeof(files) / sizeof(files[0])
                      == static_cast<int>(UnitType::Boss) + 1, "portrait table size");

        const QString roots[] = {
            QStringLiteral("src/unit/"),
            QStringLiteral("../src/unit/"),
            QCoreApplication::applicationDirPath() + QStringLiteral("/src/unit/"),
            QCoreApplication::applicationDirPath() + QStringLiteral("/../src/unit/"),
        };
        for (int i = 0; i <= static_cast<int>(UnitType::Boss); ++i)
            for (const QString& root : roots)
                if (cache[i].load(root + files[i])) break;
    }
    return cache[static_cast<int>(t)];
}

// ── 单位小图块：任意展示位通用的“立绘块” ─────────────────────────
// 暗色底框 + 阵营描边（英雄蓝/敌方红/Boss 金）+ 平滑缩放立绘；
// 立绘缺失时回退为职业色块 + 单字标签，样式与棋盘单位一致。
inline void drawUnitChip(QPainter& painter, const QRect& rect, UnitType t, bool isHero,
                         int radius = 4)
{
    QColor border = (t == UnitType::Boss) ? QColor(255, 200, 60)
                  : isHero                 ? QColor(100, 170, 255)
                                           : QColor(235, 90, 90);
    const QPixmap& pm = unitPortrait(t);
    if (!pm.isNull()) {
        painter.setBrush(QColor(22, 14, 26));
        painter.setPen(QPen(border, 1));
        painter.drawRoundedRect(rect, radius, radius);
        painter.save();
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.drawPixmap(rect.adjusted(2, 2, -2, -2), pm);
        painter.restore();
    } else {
        painter.setBrush(typeFillColor(t, isHero));
        painter.setPen(QPen(border, 1));
        painter.drawRoundedRect(rect, radius, radius);
        painter.setPen(Qt::white);
        QFont f;
        f.setPixelSize(qMax(9, rect.height() / 2));
        f.setBold(true);
        painter.setFont(f);
        painter.drawText(rect, Qt::AlignCenter, typeLabel(t));
    }
}

#endif // UNITVISUALS_H
