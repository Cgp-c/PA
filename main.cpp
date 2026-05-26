#include <QApplication>
#include "synera.h"

// ═══════════════════════════════════════════════════════════════
// 游戏主循环框架（Qt 事件驱动模型）
// ═══════════════════════════════════════════════════════════════
//
//   main()                     Synera::gameLoop()  (QTimer, ~60fps)
//   ──────────────────────     ──────────────────────────────────
//   1. 创建 QApplication        1. 计算 deltaTime
//   2. 创建主窗口 Synera         2. 若 !m_isPlayerTurn → 处理敌方回合
//       ├─ initGame()           3. update() → paintEvent()
//       │   ├─ Board 初始化         ├─ renderBoard()   8×8 方形棋盘
//       │   ├─ Hero ×3  创建        ├─ renderUnits()   单位图元
//       │   └─ Enemy ×3 创建        └─ renderUI()      状态面板
//   3. 启动 QTimer (16ms)       4. 等待下一帧
//   4. 进入 app.exec() 事件循环  ──────────────────────────────────
//
//   交互流程：
//   鼠标点击 → mousePressEvent → processPlayerClick()
//     ├─ 选中我方英雄
//     ├─ 点击空格 → 移动（相邻、我方半场）
//     └─ 点击敌兵 → 攻击（相邻判定）
//   按键 R → keyPressEvent → initGame() 重置
//
// ═══════════════════════════════════════════════════════════════
// 待修；法师技能改5 * 5 且随等级增加而增加， 刺客改成曼哈顿距离3，增加装备， 修改商店生成机制
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);



    

    Synera mainWin;
    mainWin.show();

    // 进入 Qt 事件循环 —— 此后由 QTimer::timeout 信号驱动帧更新
    return app.exec();
}
