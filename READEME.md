# Synera: Synergy Auto-Arena - 项目说明

> 高级程序设计课程 PA 项目 – 单机轻量级自走棋游戏（C++）

## 摘要

自走棋核心玩法：准备阶段（招募、布阵）→ 战斗阶段（自动战斗）→ 结算阶段（获得金币）。  
本项目实现一个 PVE 版本，使用 C++ 完成。

---

## 重要声明

- 所有代码必须独立完成，不得抄袭。
- 若基于开源项目，需主动告知并强调你做的修改（尤其是面向对象相关的重构）。
- 替换图像素材不被视为课程相关修改。
- 共四个阶段，每阶段两周，第6周检查基础功能，第8周检查扩展功能。

---

## 记号约定（建议）

- 棋盘：`B = [1,M] × [1,N]`，坐标 `(x,y)`，占用函数 `occ_r(x,y)`
- 备战区：索引集合 `K = {1,...,K}`（如 K=8）
- 玩家：`p`，拥有金币、人口上限等
- 单位：`u ∈ U`，统一用 `Unit` 表示，`owner(u)` ∈ {PlayerCtrl, EnemyCtrl}
- 羁绊标签：`traits(u) ⊆ T`，仅用于羁绊效果
- 轮次单位集合：`U_r = A_r ∪ E_r`（我方参战集合 + 敌方生成集合）
- 战斗状态：Idle, Moving, Attacking, Casting, Dead
- 索敌：先最小化欧氏距离，再按生命值、坐标序等平局规则
- 经济：金币 `g`，购买/刷新/升级人口消耗金币，上阵数 ≤ `cap(p)`
- 羁绊：对标签 `t`，计数 `n_t`，达到阈值 `Θ_t` 激活效果
- 升星：3个同名同星单位合并为高一星单位
- 装备：每个单位有装备槽（如1件）
- 存档：状态 `Σ = (p, r, phase, A_r, E_r, Bench, Shop, EquipPool)`

---

## 分数构成

| 评分项             | 占比   |
|------------------|--------|
| 基础功能（阶段1-3） | 70%    |
| 扩展内容（阶段4）   | 10%    |
| 代码风格（OOP等）   | 10%    |
| 文档与提交规范     | 10%    |

---

## 阶段一：基础布局与交互

### 功能要求

- 实现 M×N 棋盘（如 8×8），分玩家半场（下）和敌人半场（上）
- 实现一维备战区（如8格）
- 单位基类 `Unit`：HP, ATK, Range, MaxMana, Mana
- 我方/敌方统一使用 `Unit`，通过 `owner` 区分
- 玩家实体 + 敌方轮次生成（`E_r`）
- 鼠标拖拽单位在备战区和棋盘间移动，非法放置时交换或弹回
- GUI 展示棋盘、备战区、单位血条/蓝条/属性面板

### Checklist（阶段一）

- [ ] 实现棋盘、半场、地块占用规则
- [ ] 实现备战区与棋盘数据同步
- [ ] 实现 `Unit` 基类（HP/ATK/Range/MaxMana/Mana）
- [ ] 统一 `Unit` 类型，用 `owner` 区分敌我
- [ ] 实现玩家实体与轮次敌方生成
- [ ] 实现拖拽摆放与非法处理（交换/回弹）
- [ ] GUI 展示棋盘/备战区/单位信息

---

## 阶段二：战斗逻辑

### 功能要求

- 准备阶段 → 战斗阶段 → 结算阶段 循环
- 战斗开始自动生成敌方阵容（随关卡增强）
- 单位状态机（Idle, Moving, Attacking, Casting, Dead）
- 索敌：欧氏距离最近 → 平局规则（生命值低 → 从左向右 → 从下到上）
- 寻路与碰撞：不能穿过单位，需绕行（BFS / A*），同一格仅一个单位
- 攻击与技能：普攻回蓝，满蓝释放技能（多态，实现3-5种英雄）
- 胜负：一方全灭则结束，扣血或通关

### 参考数值

| 项目             | 数值建议                         |
|----------------|--------------------------------|
| 初始法力         | 0（可自定义）                    |
| 普攻回蓝         | 10                              |
| 最大法力         | 60（可不同）                    |
| 攻击速度         | 每60帧攻击1次                   |
| 移动速度         | 每20帧移动1格                   |
| 1星单位 ATK/HP   | 20-50 / 200-500                |
| 2星属性提升      | 1.5~2倍                        |
| 敌方每3轮属性提升 | 约20%                          |

### Checklist（阶段二）

- [ ] 完成三阶段循环与轮次推进
- [ ] 敌方按关卡配置生成并随轮次增强
- [ ] 实现单位状态机
- [ ] 实现索敌规则
- [ ] 实现寻路、阻挡与防重叠碰撞
- [ ] 实现普攻、回蓝、技能多态（3-5英雄）与胜负结算

---

## 阶段三：完整经济与成长系统

### 功能要求

- 金币系统：每关获得基础金币
- 商店：5个随机单位，可购买、刷新
- 人口系统：初始上限3-4，可花费金币升级（每级+1人口）
- 羁绊系统：4-6种职业/种族，至少包含2种属性光环类 + 1种机制改变类
- 升星：3个同名同星合并为2星，属性提升
- 装备掉落（概率），基础装备至少4种：

| 装备       | 效果                 |
|------------|----------------------|
| 铁剑       | 攻击力+15            |
| 锁子甲     | 生命值+150           |
| 急速手套   | 攻击速度提升20%      |
| 蓝水晶     | 最大法力值-30        |

- 装备穿戴：每单位最多1件（2星可扩展为2件）
- 存档/读档：序列化状态到文件，完全恢复

### GUI 补充要求

- 显示玩家血量/金币/人口数
- 显示商店招募区
- 显示装备栏、羁绊、星级
- 显示当前轮次/阶段/准备时间

### Checklist（阶段三）

- [ ] 实现金币系统、关卡奖励与商店（5个招募位）
- [ ] 实现购买、刷新与备战区落位
- [ ] 实现人口升级与上阵限制
- [ ] 实现4-6种羁绊（≥2属性光环 + 1机制改变）
- [ ] 实现升星（3合1）与2星属性提升
- [ ] 实现装备掉落、穿戴限制与至少4种装备
- [ ] 实现存档/读档
- [ ] GUI完整展示经济、商店、羁绊、星级、轮次等

---

## 阶段四：扩展功能（10%）

选择至少一个实现：

- 高级经济系统：利息（每10金币+1）、连胜/连败奖励
- 装备合成树：两件基础装备合成为高级装备（如复活甲）
- 视听效果：背景音乐、攻击音效、弹道飞行物、技能特效
- 局域网联机：PvP，Socket 传输镜像军队
- 战争迷雾/视野系统
- 智能索敌：按威胁程度选择目标

### Checklist（阶段四）

- [ ] 实现至少1个扩展功能
- [ ] 与现有系统正确集成
- [ ] 完成对应GUI/交互展示
- [ ] 在README中说明扩展设计

---

## 实验指南

### GUI 与 Starter Code

- 推荐 Qt 或 SDL2（提供简易网格 Demo 参考）
- 不限制具体图形库，界面精美度不影响基础分

### 基于帧的逻辑与状态机

- 60fps，每帧调用所有单位的 `update()`
- 为每个单位实现有限状态机（FSM）

### 寻路与可达性

- BFS / A*，由于单位动态移动，需定期重新计算路径（re-path）
- 处理围堵情况

### 目标锁定

- 欧氏距离最近，平局规则（低血量 → 坐标序）

### 素材获取

- 免费资源：Kenney, OpenGameArt, itch.io, CraftPix
- 可用纯色块+文字代替图像，不影响评分

### 如何提问

- 提供上下文、已尝试的方法、错误信息
- 先搜索或问 AI
- 单次聚焦一个问题

### 项目提交

- 压缩包（.zip/.tar.gz），剔除 build/、*.o
- 包含 README.md（基本信息、文件树、核心类、算法、辅助函数）
- 单独一节“AI使用说明”：说明如何用AI辅助，并解释至少两个AI生成的核心模块

### 代码风格

- OOP：继承与多态
- 使用 STL
- 异常处理（存档/读档用 try-catch）
- 整洁格式（推荐 clang-format）

---

## 说明与致谢

项目玩法灵感来自《The Last Flame》《云顶之弈》，经教学化抽象，重点训练面向对象设计能力。

---

## 当前代码架构（v0.1 — Starter 框架）

基于 **Qt6 + C++17 + MinGW**，采用 **事件驱动游戏循环**（QTimer @ ~60fps），在 `Synera` 主窗口内完成 8×8 方形棋盘渲染、鼠标点选交互与回合制战斗。

### 文件树

```text
PA/
├─ CMakeLists.txt
├─ README.md
├─ synera.ui                  # Qt Designer UI 布局
├─ main.cpp                   # 入口 + 游戏主循环框架说明
├─ synera.h / synera.cpp      # 主窗口、棋盘渲染、回合逻辑
├─ unit.h / unit.cpp          # 单位基类
├─ hero.h / hero.cpp          # 我方英雄（派生类）
├─ enemy.h / enemy.cpp        # 敌方单位（派生类）
├─ weapon.h / weapon.cpp      # 装备类
└─ board.h / board.cpp        # 8×8 棋盘数据
```

---

### 1. `Position` 结构体（`unit.h`）

坐标抽象，表示棋盘上的格子位置。

| 成员 | 类型 | 说明 |
|------|------|------|
| `x` | `int` | 列坐标 (0–7) |
| `y` | `int` | 行坐标 (0–7) |
| `operator==` | `bool` | 比较两个位置是否相同 |

---

### 2. `Weapon` — 装备类（`weapon.h/cpp`）

描述一件可穿戴装备，决定单位的攻击力加成。

| 成员 / 方法 | 类型 | 说明 |
|-------------|------|------|
| `m_name` | `std::string` | 装备名称（如 "Iron Sword"） |
| `m_damage` | `int` | 攻击力加成数值 |
| `Weapon(name, damage)` | 构造函数 | 创建指定名称与攻击力的装备 |
| `getName()` | `std::string` | 获取装备名称 |
| `getDamage()` | `int` | 获取攻击力加成 |

**接口**：`Unit` 通过 `setEquipment(Weapon*)` / `getEquipment()` 穿戴 / 查询装备，`attack()` 时读取装备攻击力。

---

### 3. `Unit` — 单位抽象基类（`unit.h/cpp`）

所有战斗实体的公共基类，定义属性与纯虚接口，由 `Hero` / `Enemy` 继承。

| 成员 / 方法 | 类型 | 说明 |
|-------------|------|------|
| `m_name` | `std::string` | 单位名称 |
| `m_hp` | `int` | 当前生命值 |
| `m_maxHp` | `int` | 最大生命值 |
| `m_pos` | `Position` | 当前棋盘位置 |
| `m_equipment` | `Weapon*` | 穿戴的装备（可为 `nullptr`） |
| `m_disappeared` | `bool` | 是否已消失（死亡后移除出场） |
| `Unit(name, hp, maxHp, x, y)` | 构造函数 | 初始化所有属性，装备置空，未消失 |
| `attack(target)` | `virtual void = 0` | **纯虚函数** — 攻击目标单位，子类分别实现 |
| `isDead()` | `bool` | HP ≤ 0 即死亡 |
| `isDisappeared()` | `bool` | 是否已从场上移除 |
| `takeDamage(damage)` | `void` | 扣减 HP（不低于 0） |
| `setDisappeared(flag)` | `void` | 标记消失状态 |
| `getName()` / `getHp()` / `getMaxHp()` | getter | 属性访问 |
| `getPosition()` | `Position` | 获取当前坐标 |
| `getEquipment()` | `Weapon*` | 获取装备指针 |
| `setPosition(x, y)` | `void` | 更新坐标 |
| `setEquipment(weapon)` | `void` | 穿戴装备 |
| `setHp(hp)` | `void` | 设置生命值 |

**多态接口**：`attack()` 在 `Hero` 中基于装备伤害计算（默认 10），在 `Enemy` 中同样基于装备（默认 8），攻击后若目标死亡则自动调用 `setDisappeared(true)`。

---

### 4. `Hero` — 我方单位（`hero.h/cpp`，继承 `Unit`）

| 成员 | 说明 |
|------|------|
| `Hero(name, hp, maxHp, x, y)` | 构造函数，委托 `Unit` |
| `attack(target)` | 以装备伤害（或默认 10）攻击目标，目标死亡则消失 |

---

### 5. `Enemy` — 敌方单位（`enemy.h/cpp`，继承 `Unit`）

| 成员 | 说明 |
|------|------|
| `Enemy(name, hp, maxHp, x, y)` | 构造函数，委托 `Unit` |
| `attack(target)` | 以装备伤害（或默认 8）攻击目标，目标死亡则消失 |

---

### 6. `Board` — 8×8 方形棋盘（`board.h/cpp`）

纯数据层，管理格子占用与合法性检查。

| 成员 / 方法 | 说明 |
|-------------|------|
| `SIZE = 8` | `static constexpr`，棋盘边长 |
| `m_grid[8][8]` | `Unit*` 二维数组，`nullptr` 表示空格 |
| `Board()` | 初始化全空 |
| `placeUnit(unit, x, y)` | 若位置合法且为空则放置，更新 unit 的坐标 |
| `getUnitAt(x, y)` | 返回格子上的单位指针（`nullptr` 表示空） |
| `isOccupied(x, y)` | 该格是否已有单位 |
| `removeUnit(x, y)` | 清空指定格 |
| `isValidPosition(x, y)` | 坐标是否在 [0, SIZE) 范围内 |
| `isPlayerHalf(y)` | 是否在我方半场（y ≥ SIZE/2，即 row 4–7） |
| `isEnemyHalf(y)` | 是否在敌方半场（y < SIZE/2，即 row 0–3） |
| `clear()` | 清空全部格子 |

**接口角色**：`Synera` 在 `placeUnit / removeUnit` 时仅通过 `Board` 操作；`Board` 不持有单位所有权（`Unit*` 为裸指针，生命周期由 `Synera::m_units` 管理）。

---

### 7. `Synera` — 游戏主窗口（`synera.h/cpp`）

继承 `QMainWindow`，是全部游戏逻辑的集中入口。**同时承担 Controller（回合调度 / 规则校验）和 View（paintEvent 渲染）职责。**

#### 7.1 成员变量

| 变量 | 类型 | 说明 |
|------|------|------|
| `ui` | `Ui::MainWindow*` | Qt Designer UI 指针 |
| `m_gameTimer` | `QTimer*` | 16ms 定时器，驱动 `gameLoop()` |
| `m_frameClock` | `QElapsedTimer` | 帧计时，计算 deltaTime |
| `m_board` | `Board` | 棋盘数据 |
| `m_units` | `vector<unique_ptr<Unit>>` | 所有单位的生命周期持有者 |
| `m_weapons` | `vector<unique_ptr<Weapon>>` | 所有装备的生命周期持有者 |
| `m_selectedUnit` | `Unit*` | 当前选中的单位（裸指针，不持有） |
| `m_isPlayerTurn` | `bool` | 是否玩家回合 |
| `m_gameOver` | `bool` | 游戏是否结束 |

#### 7.2 常量

| 常量 | 值 | 说明 |
|------|-----|------|
| `CELL_SIZE` | 80 | 每格像素大小 |
| `BOARD_OFFSET_X` | 200 | 棋盘左上角 X 偏移 |
| `BOARD_OFFSET_Y` | 80 | 棋盘左上角 Y 偏移 |
| `BOARD_PIXEL_SIZE` | 640 | 棋盘总像素边长 (80×8) |

#### 7.3 核心方法

**初始化**

| 方法 | 说明 |
|------|------|
| `initGame()` | 重置全部状态：清空棋盘、创建单位、重置回合标记 |
| `initUnits()` | 创建 3 个 Hero + 3 个 Enemy + 4 件 Weapon，放置到 `m_board` 上，所有权移入 `m_units` / `m_weapons` |

**游戏主循环**

| 方法 | 说明 |
|------|------|
| `gameLoop()` [slot] | QTimer 回调：计算 deltaTime → 非玩家回合则调用 `processEnemyTurn()` → `update()` 触发重绘 |

**渲染（paintEvent 调用链）**

| 方法 | 说明 |
|------|------|
| `paintEvent(event)` | Qt 绘制入口，依次调用三个 render 函数 |
| `renderBoard(painter)` | 绘制 8×8 方形网格：半场底色（玩家蓝灰 / 敌方暗红）、选中高亮绿色、坐标标注 |
| `renderUnits(painter)` | 遍历棋盘绘制单位：圆角矩形 + 名字 + HP 条（颜色渐变）+ HP 数值 + 装备名 |
| `renderUI(painter)` | 右侧面板：回合状态提示、操作说明、存活单位列表及属性 |

**鼠标交互**

| 方法 | 说明 |
|------|------|
| `mousePressEvent(event)` | Qt 鼠标事件入口，仅在玩家回合且未结束时响应 |
| `processPlayerClick(mousePos)` | 将像素坐标转棋盘坐标，分 5 种情况：① 选中我方英雄 ② 切换选中 ③ 攻击相邻敌人 ④ 移动至相邻空格（仅限我方半场） ⑤ 点空取消选中 |

**敌方 AI**

| 方法 | 说明 |
|------|------|
| `processEnemyTurn()` | 收集存活 Enemy，每个依次：若与英雄相邻则攻击，否则向最近英雄移动 1 步（曼哈顿方向） |
| `findNearestHero(from)` | 欧氏距离最近且未死亡/消失的 Hero 位置；无存活 Hero 返回 `(-1,-1)` |

**规则辅助**

| 方法 | 说明 |
|------|------|
| `isAdjacent(a, b)` | 曼哈顿距离 = 1（上下左右相邻） |
| `checkWinCondition()` | 检查是否一方全灭 → `m_gameOver = true` |
| `cellRect(x, y)` | 棋盘格 → 像素矩形（QRect） |
| `findUnitAt(x, y)` | 查询棋盘上的单位（委托 Board） |

**键盘**

| 方法 | 说明 |
|------|------|
| `keyPressEvent(event)` | 按 R 键重置游戏（调用 `initGame()`） |

---

### 8. `main.cpp` — 游戏入口与主循环框架

| 职责 | 说明 |
|------|------|
| 创建 `QApplication` | Qt 事件系统基础 |
| 创建 `Synera` 主窗口 | 在其中初始化棋盘、单位、定时器 |
| `mainWin.show()` | 显示窗口 |
| `app.exec()` | 进入 Qt 事件循环 —— 此后由 `QTimer::timeout` 信号驱动帧更新 |

**游戏主循环链路**：

```
QTimer::timeout (16ms)
  → Synera::gameLoop()
    → processEnemyTurn()   [非玩家回合时]
    → update()
      → paintEvent()
        → renderBoard() + renderUnits() + renderUI()
```

---

### 9. 全局变量

**本项目无全局变量。** 所有游戏状态封装在 `Synera` 类的成员中：
- 棋盘数据 → `Synera::m_board`
- 单位生命周期 → `Synera::m_units`
- 装备生命周期 → `Synera::m_weapons`
- 回合状态 → `Synera::m_isPlayerTurn`, `Synera::m_gameOver`
- 选中状态 → `Synera::m_selectedUnit`

---

### 10. 类间关系（UML 概要）

```
Unit (abstract)  ◄──── Weapon*  (装备，聚合)
  ▲
  ├── Hero    (我方，attack 默认伤害 10)
  └── Enemy   (敌方，attack 默认伤害 8)

Board (8×8)  ◄──── Unit*[8][8]  (棋盘持有单位裸指针)
  ▲
  │ 使用
  │
Synera (QMainWindow)
  ├── 持有 Board
  ├── 持有 vector<unique_ptr<Unit>>   (生命周期管理)
  ├── 持有 vector<unique_ptr<Weapon>> (生命周期管理)
  ├── 持有 QTimer                     (游戏循环驱动)
  └── 实现 paintEvent / mousePressEvent / keyPressEvent
```

---

### 11. 与 PA 阶段一 Checklist 的对应

| PA 要求 | 当前实现 |
|---------|---------|
| 8×8 棋盘 + 半场 | `Board` 类 + `isPlayerHalf()` / `isEnemyHalf()` |
| 单位基类 Unit（HP 等） | `Unit` 抽象基类：HP / maxHp / name / position / equipment |
| 我方/敌方通过 owner 区分 | 通过派生类 `Hero` / `Enemy` 区分（`dynamic_cast` 判定） |
| 鼠标拖拽 | 改为 **点选式交互**（click-to-select, click-to-act），更简洁稳定 |
| GUI 展示棋盘、血条等 | `renderBoard()` / `renderUnits()` / `renderUI()` 完整渲染 |
| 回合制战斗 | 玩家行动 → 敌方 AI 自动响应 → 胜负判定 |

> **关于交互方式的说明**：当前采用点选而非拖拽。拖拽实现需搭配 `mouseMoveEvent` 与实时碰撞检测，对 Starter 框架来说点选更直观且易调试，可在后续阶段切换为拖拽。