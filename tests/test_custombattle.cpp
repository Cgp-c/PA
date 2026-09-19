// CustomBattleWindow 校验与交互的 headless 测试：
// 用合成鼠标事件真实驱动控件，验证数量上限锁定、类型/星级循环、删除/添加。
//
// 编译（在项目根目录，Qt 安装在 C:/Qt/6.11.2/mingw_64 时）：
//   mkdir -p build && cd build
//   C:/Qt/6.11.2/mingw_64/bin/moc.exe ../custombattlewindow.h -o moc_custombattlewindow.cpp
//   g++ -std=c++17 -DUNICODE -DQT_NO_DEBUG //     -I. -I.. -I<Qt>/include -I<Qt>/include/QtWidgets -I<Qt>/include/QtGui -I<Qt>/include/QtCore //     ../tests/test_custombattle.cpp ../custombattlewindow.cpp moc_custombattlewindow.cpp //     -o test_custombattle.exe -L<Qt>/lib -lQt6Widgets -lQt6Gui -lQt6Core -lole32 -luuid -lgdi32 -lwinmm -lws2_32 -ldwmapi
// 运行（默认 windows 平台会短暂闪现窗口；本机 offscreen/minimal 平台初始化会挂起，勿用）：
//   PATH="<Qt>/bin:$PATH" ./test_custombattle.exe
#include <QApplication>
#include <QMouseEvent>
#include <cstdio>
#include "custombattlewindow.h"

static int failures = 0;
static void check(bool cond, const char* name)
{
    printf("%-56s %s\n", name, cond ? "PASS" : "FAIL");
    if (!cond) ++failures;
}

static void clickAt(QWidget* w, int x, int y)
{
    QPoint pos(x, y);
    QMouseEvent press(QEvent::MouseButtonPress, pos, w->mapToGlobal(pos),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(w, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, pos, w->mapToGlobal(pos),
                        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(w, &release);
}

// 行 0 控件中心坐标（与 custombattlewindow.cpp 布局常量对应）
static constexpr int ROW0_Y = 118 + 21;
static constexpr int TYPE_X = 20 + 92 / 2;
static constexpr int STAR_X = 20 + 92 + 8 + 76 / 2;
static constexpr int PLUS_X = 20 + 92 + 8 + 76 + 8 + 110 - 15;   // + 段中心
static constexpr int MINUS_X = 20 + 92 + 8 + 76 + 8 + 15;        // - 段中心
static constexpr int DEL_X = 20 + 92 + 8 + 76 + 8 + 110 + 8 + 30;
static constexpr int ADD_Y = 118 + 50 + 15;

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    CustomBattleWindow w;
    w.show();
    app.processEvents();

    // ── 默认状态 ──
    check(w.specs().size() == 1, "default: one row");
    check(w.specs()[0].type == (int)UnitType::Warrior, "default: type Warrior");
    check(w.totalEnemies() == 1, "default: total == 1");
    check(w.isValid(), "default: valid");

    // ── 数量上限锁定：连点 + 40 次，总数不得超过 32 ──
    for (int i = 0; i < 40; ++i) clickAt(&w, PLUS_X, ROW0_Y);
    check(w.specs()[0].count == CustomBattleWindow::MAX_ENEMIES,
          "plus x40: count capped at MAX_ENEMIES(32)");
    check(w.totalEnemies() == 32, "plus x40: total == 32");
    check(w.isValid(), "plus x40: still valid at cap");

    // ── 减量：2 次后 30 ──
    clickAt(&w, MINUS_X, ROW0_Y);
    clickAt(&w, MINUS_X, ROW0_Y);
    check(w.specs()[0].count == 30, "minus x2: count == 30");

    // ── 类型循环：战士→法师→辅助→刺客→Boss ──
    clickAt(&w, TYPE_X, ROW0_Y);
    check(w.specs()[0].type == (int)UnitType::Mage, "type cycle 1: Mage");
    clickAt(&w, TYPE_X, ROW0_Y);
    check(w.specs()[0].type == (int)UnitType::Support, "type cycle 2: Support");
    clickAt(&w, TYPE_X, ROW0_Y);
    clickAt(&w, TYPE_X, ROW0_Y);
    check(w.specs()[0].type == (int)UnitType::Boss, "type cycle 4: Boss");

    // ── Boss 星级锁定：点击星级不应变化 ──
    clickAt(&w, STAR_X, ROW0_Y);
    clickAt(&w, STAR_X, ROW0_Y);
    check(w.specs()[0].star == 0, "Boss: star locked at 0");

    // ── 普通职业星级循环 0→3 ──
    clickAt(&w, TYPE_X, ROW0_Y);              // Boss → Warrior
    for (int i = 0; i < 5; ++i) clickAt(&w, STAR_X, ROW0_Y);
    check(w.specs()[0].star == 1, "star cycle x5 (0->3->0->1): star == 1");

    // ── 删除后：无配置 → 非法 ──
    clickAt(&w, DEL_X, ROW0_Y);
    check(w.specs().isEmpty(), "delete: row removed");
    check(w.totalEnemies() == 0, "delete: total == 0");
    check(!w.isValid(), "delete: invalid when empty");

    // ── 重新添加 ──
    clickAt(&w, 200, ADD_Y);
    check(w.specs().size() == 1, "add: one row back");
    check(w.isValid(), "add: valid again");

    printf("\n%s\n", failures ? "SOME CHECKS FAILED" : "ALL CHECKS PASSED");
    return failures ? 1 : 0;
}
