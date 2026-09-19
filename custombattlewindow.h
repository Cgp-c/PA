#ifndef CUSTOMBATTLEWINDOW_H
#define CUSTOMBATTLEWINDOW_H

#include <QWidget>
#include <QVector>
#include <QRect>
#include "board.h"

// 自定义难度窗口 — 独立于关卡系统的对手编辑器。
// 与装备合成树一样：非模态、独立渲染层（自己的 paintEvent）、关闭仅隐藏。
// 玩家可配置任意敌方编成（类型/星级/数量，含 Boss），
// 数量总和受敌方半场容量上限约束。
class CustomBattleWindow : public QWidget {
    Q_OBJECT

public:
    // 一组敌方配置：类型 + 星级 + 数量
    struct EnemySpec {
        int type;    // UnitType 枚举值（0-3 职业，4 = Boss）
        int star;    // 0-3 整星（Boss 固定属性，忽略此值）
        int count;   // 该组数量（>=1）
    };

    explicit CustomBattleWindow(QWidget* parent = nullptr);
    ~CustomBattleWindow() override;

    QVector<EnemySpec> specs() const { return m_specs; }
    int totalEnemies() const;

    // 敌方半场（8x8 棋盘的上半 4 行）可放置上限
    static constexpr int MAX_ENEMIES = Board::SIZE * (Board::SIZE / 2);
    static constexpr int MAX_ROWS = 8;   // 最多同时配置的组数

    bool isValid() const;   // 0 < 总数 <= MAX_ENEMIES 且所有字段在合法范围

signals:
    void startRequested();  // 点击“开始自定义战斗”且配置合法时发出

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    QVector<EnemySpec> m_specs;

    // 每帧重建的点击热区
    QVector<QRect> m_typeRects, m_starRects, m_minusRects, m_plusRects, m_delRects;
    QRect m_addRect, m_clearRect, m_startRect;

    void resetToDefault();
    static QString typeButtonText(int type);
    static QString starButtonText(int type, int star);
    static QColor  typeButtonColor(int type);
};

#endif // CUSTOMBATTLEWINDOW_H
