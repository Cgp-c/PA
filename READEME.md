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

## 当前代码架构（v0.3 — 拖拽布阵 + 全自动战斗 + 法力技能）

基于 **Qt6 + C++17 + MinGW**，采用 **事件驱动游戏循环**（QTimer @ ~60fps），实现拖拽布阵与全自动战斗。

### 文件树

```text
PA/
├─ CMakeLists.txt
├─ README.md
├─ synera.ui                  # Qt Designer UI 布局
├─ main.cpp                   # 入口
├─ synera.h / synera.cpp      # 主窗口、渲染、游戏循环、自动战斗
├─ unit.h / unit.cpp          # 单位抽象基类 + UnitType 枚举 + 坐标/距离工具
├─ hero.h / hero.cpp          # Hero 基类 + WarriorHero / MageHero / SupportHero
├─ enemy.h / enemy.cpp        # Enemy 基类 + WarriorEnemy / MageEnemy / SupportEnemy
├─ weapon.h / weapon.cpp      # 装备类
└─ board.h / board.cpp        # 8×8 棋盘数据
```

---

### 更新一：拖拽放置（>50% 面积判定） + 左侧备战池

**需求**：玩家从左侧 Unit Pool 拖拽单位到棋盘，放下时若单位矩形与目标格重叠面积 >50% 则吸附，否则弹回。

#### `Position` 结构体（`unit.h`）

| 成员 | 类型 | 说明 |
|------|------|------|
| `x` | `int` | 列坐标 (0–7) |
| `y` | `int` | 行坐标 (0–7) |
| `operator==` | `bool` | 比较两个位置是否相同 |

#### `manhattanDist(a, b)` 全局函数（`unit.h`）

| 签名 | 说明 |
|------|------|
| `int manhattanDist(const Position&, const Position&)` | 曼哈顿距离 = \|dx\| + \|dy\|，用于索敌和移动判定 |

#### `PoolSlot` 结构体（`synera.h`）

备战池中的一条记录，表示一种可放置的单位类型及其剩余数量。

| 成员 | 类型 | 说明 |
|------|------|------|
| `type` | `UnitType` | 单位角色（Warrior / Mage / Support） |
| `count` | `int` | 该类型剩余可放置数量 |

#### 拖拽相关方法（`synera.cpp`）

| 方法 | 说明 |
|------|------|
| `generateUnitPool()` | 随机生成 Unit Pool：战士 2~3、法师 1~2、辅助 1~2 |
| `createUnitFromPool(type, isHero)` | 按类型实例化对应的 WarriorHero/MageHero/SupportHero |
| `processDragStart(mousePos)` | 按下时：若命中 Pool 且有库存 → 创建单位开始拖拽并扣减数量；若命中棋盘上英雄 → 从棋盘拿起 |
| `processDrop(mousePos)` | 释放时：放回 Pool → 返还数量/移除单位；放到棋盘 → 遍历我方半场，计算 `unitRect.intersected(cellRect)`，重叠面积 > `CELL_SIZE² / 2` 且最高者吸附 |

#### 左侧备战池渲染

| 方法 | 说明 |
|------|------|
| `renderBenchPool(painter)` | 在 `(10, 80)` 绘制 Pool 列：每个槽位 170×100 px，显示角色色块（战/法/辅）、名称、剩余数量、属性摘要 |
| `poolSlotRect(index)` | Pool 槽位 → QRect |
| `findPoolSlotAt(pixel)` | 像素坐标 → Pool 槽位索引，-1 表示未命中 |

#### 拖拽幽灵

| 方法 | 说明 |
|------|------|
| `renderDragGhost(painter)` | 以 75% 不透明度在鼠标位置绘制被拖拽单位的色块 + 角色标签 |

---

### 更新二：三职业类层次 + 全自动战斗

**需求**：引入战士/法师/辅助三种职业，每种职业在 Hero 和 Enemy 下各有一个子类。战斗开始后完全自动，每 ~0.3s 一个 tick，所有单位同时索敌、移动、攻击或治疗。

#### `UnitType` 枚举（`unit.h`）

```cpp
enum class UnitType { Warrior, Mage, Support };
```

#### `Unit` 基类新增成员（`unit.h/cpp`）

| 成员 / 方法 | 类型 | 说明 |
|-------------|------|------|
| `m_type` | `UnitType` | 职业类型 |
| `getType()` | `UnitType` | 获取职业 |
| `getAttackRange()` | `virtual int = 0` | 攻击距离（战士=1, 法师=4, 辅助=1） |
| `getAttackDamage()` | `virtual int = 0` | 攻击伤害（战士=20, 法师=10, 辅助=0） |
| `getHealAmount()` | `virtual int` | 治疗量（仅辅助=20, 其余=0） |
| `canHeal()` | `virtual bool` | 是否可治疗（仅辅助=true） |
| `heal(amount)` | `void` | 回复 HP，不超过 m_maxHp（为装备提高生命值上限预留接口） |
| `setMaxHp(maxHp)` | `void` | 设置最大生命值（装备接口预留） |
| `attack(target)` | `virtual void` | 改为非纯虚：基础伤害 = `getAttackDamage() + 装备加成` |

#### 职业子类（`hero.h/cpp` + `enemy.h/cpp`）

```
Unit
├── Hero
│   ├── WarriorHero   "A-Warrior"   HP:100  ATK:20  Range:1
│   ├── MageHero      "A-Mage"      HP:50   ATK:10  Range:4
│   └── SupportHero   "A-Support"   HP:80   Heal:20 Range:1  canHeal=true
└── Enemy
    ├── WarriorEnemy  "E-Warrior"   HP:100  ATK:20  Range:1
    ├── MageEnemy     "E-Mage"      HP:50   ATK:10  Range:4
    └── SupportEnemy  "E-Support"   HP:80   Heal:20 Range:1  canHeal=true
```

每个子类仅重写 `getAttackRange()` / `getAttackDamage()` / `getHealAmount()` / `canHeal()`。攻击逻辑统一在 `Unit::attack()` 中实现。

#### 占位符渲染（`synera.cpp`）

| 函数 | 说明 |
|------|------|
| `typeFillColor(type, isHero)` | 战士=橙、法师=紫、辅助=绿；敌方颜色略深 |
| `typeLabel(type)` | 返回中文标签："战"/"法"/"辅" |

`renderUnits()` 中每个单位被绘制为带角色标签的色块 + 名称 + HP 条。

#### 自动战斗系统（`synera.cpp`）

**常量**

| 常量 | 值 | 说明 |
|------|-----|------|
| `TICK_INTERVAL` | 18 | 战斗 tick 间隔帧数（18×16ms ≈ 0.29s） |
| `POOL_X / POOL_Y / POOL_SLOT_H / POOL_WIDTH` | 10 / 80 / 100 / 170 | Pool 面板布局 |

**战斗 tick 方法**

| 方法 | 说明 |
|------|------|
| `processCombatTick()` | 每 TICK_INTERVAL 帧调用一次：遍历棋盘上所有存活单位，计算行动（攻击/治疗/移动），执行移动，判定胜负 |

**行动逻辑（`processCombatTick` 内部）**

| 职业 | 行动 |
|------|------|
| 战士 | `findNearestEnemyFor()` 找最近敌人 → 若相邻则 `attack()`，否则 `moveStepToward()` 移动 1 步 |
| 法师 | 同上，但攻击距离 ≤4 即可攻击（`canAttack()` 用 `manhattanDist` 判定） |
| 辅助 | `findHealTarget()` 找最近受伤队友 → 相邻则 `heal()`；若无人受伤则按战士逻辑移动（出界则不移动）；若找不到敌人也不移动 |

**索敌与治疗选择**

| 方法 | 说明 |
|------|------|
| `findNearestEnemyFor(unit)` | 遍历棋盘找不同阵营的最近单位（曼哈顿距离） |
| `findHealTarget(support)` | 遍历棋盘找同阵营 HP < maxHP 的最近单位。距离相同时：优先战士 > 法师 > 辅助；角色相同则优先横坐标更近 |
| `canAttack(attacker, target)` | `manhattanDist ≤ attacker.getAttackRange()` |

**移动**

| 方法 | 说明 |
|------|------|
| `moveStepToward(from, to)` | 单步移动：优先 x 方向，若被占/出界则尝试 y 方向，均不可行则原地不动。返回目标 Position（可能与 from 相同） |

**胜负判定**

| 方法 | 说明 |
|------|------|
| `checkWinCondition()` | 遍历 m_units：若一方全员阵亡 **或** 一方只剩辅助（无战士/法师）→ `m_gameOver = true` |

#### 游戏主循环

```
QTimer::timeout (16ms)
  → Synera::gameLoop()
    → if (Battle && !gameOver) m_combatTickCounter++
      → if (>= TICK_INTERVAL) processCombatTick()
    → update()
      → paintEvent()
        → renderBoard() + renderUnits() + renderBenchPool() + renderDragGhost() + renderUI()
```

#### `Synera` 成员变量（当前完整）

| 变量 | 类型 | 说明 |
|------|------|------|
| `m_board` | `Board` | 8×8 棋盘数据 |
| `m_units` | `vector<unique_ptr<Unit>>` | 所有单位生命周期持有者 |
| `m_weapons` | `vector<unique_ptr<Weapon>>` | 所有装备生命周期持有者 |
| `m_unitPool` | `vector<PoolSlot>` | 左侧备战池（类型 + 数量） |
| `m_phase` | `GamePhase` | Placement / Battle |
| `m_gameOver` | `bool` | 战斗是否结束 |
| `m_combatTickCounter` | `int` | 战斗帧计数器 |
| `m_draggedUnit` | `Unit*` | 当前拖拽单位（裸指针） |
| `m_dragCurrentPos` | `QPoint` | 拖拽中鼠标位置 |
| `m_dragFromPoolIndex` | `int` | 从 Pool 哪一槽拖出（-1 = 从棋盘） |

#### `Synera` 常量

| 常量 | 值 | 说明 |
|------|-----|------|
| `CELL_SIZE` | 80 | 每格像素 |
| `BOARD_OFFSET_X/Y` | 200 / 80 | 棋盘左上角 |
| `POOL_X/Y` | 10 / 80 | Pool 面板左上角 |
| `POOL_SLOT_H` | 100 | 每个 Pool 槽高度 |
| `POOL_WIDTH` | 170 | Pool 面板宽度 |
| `TICK_INTERVAL` | 18 | 战斗 tick 帧间隔 |

---

### 更新三：法力值与技能系统

**需求**：每次攻击或治疗积攒 1 点法力值，满法力（5 点）时自动释放职业技能并清空法力。为每个单位添加燃烧 debuff 系统。

#### `Unit` 基类新增法力/技能/燃烧成员（`unit.h/cpp`）

| 成员 / 方法 | 类型 / 签名 | 说明 |
|-------------|-------------|------|
| `MAX_MANA` | `static constexpr int` | 最大法力值 = 5 |
| `m_mana` | `int` | 当前法力值 |
| `m_burning` | `bool` | 是否处于燃烧状态 |
| `m_burningTurns` | `int` | 燃烧剩余回合数 |
| `getMana()` | `int` | 获取当前法力值 |
| `getMaxMana()` | `int` | 获取最大法力值（返回 `MAX_MANA`） |
| `gainMana()` | `void` | 法力值 +1，不超过 `MAX_MANA` |
| `resetMana()` | `void` | 法力值归零 |
| `useSkill(board, allUnits)` | `virtual void = 0` | 纯虚函数：释放职业技能 |
| `isBurning()` | `bool` | 是否燃烧中 |
| `getBurningTurns()` | `int` | 获取燃烧剩余回合 |
| `applyBurning(turns)` | `void` | 施加燃烧 debuff（覆盖旧值） |
| `tickBurning()` | `void` | 燃烧结算：扣除 10 HP，减少 1 回合；回合归零则熄灭 |

#### 职业技能实现（`hero.cpp` / `enemy.cpp`）

| 职业 | 技能 | 实现细节 |
|------|------|---------|
| 战士 | 对最近敌方单位造成 **80** 固定伤害 | 遍历 `allUnits`，用 `dynamic_cast` 过滤敌对阵营，曼哈顿距离最近者受到 80 伤害；若击杀则调用 `board.removeUnit()` |
| 法师 | 周围 **3×3** 范围内所有敌方单位施加燃烧（**10 伤害/回合 × 4 回合**） | 遍历 `allUnits`，\|dx\| ≤ 1 且 \|dy\| ≤ 1 的敌对单位调用 `applyBurning(4)`；燃烧独立于施法者生命周期 |
| 辅助 | 全场生命值绝对值最低的 **2** 个单位各回复 **30** HP | 按 `getHp()` 升序排列，跳过死亡/消失的单位，治疗前 2 个；无距离限制 |

#### 战斗 tick 中的技能判定（`synera.cpp` — `processCombatTick`）

```
for each alive unit:
    if unit.mana >= MAX_MANA:
        unit.useSkill(board, alive)   // 释放技能
        unit.resetMana()              // 法力归零
        continue                      // 本回合不执行普通行动
    // 否则执行普通攻击/治疗，成功后 gainMana()
```

另外每 tick 开始时，先对所有存活单位调用 `processBurningTick(alive)` 结算燃烧伤害。燃烧在单位死亡后不再触发，但 debuff 在单位存活期间独立于施法者。

#### 法力值显示（`synera.cpp` — `renderUnits`）

每个单位 HP 条上方绘制 5 个圆形法力指示器：

| 参数 | 值 | 说明 |
|------|-----|------|
| 圆的半径 | 4 px | 小圆点 |
| 间距 | 10 px | 圆心之间 |
| 填充圆 | `QColor(240, 240, 240)` | 表示已攒法力 |
| 空心圆 | `QColor(60, 60, 60)` | 表示未攒法力 |
| 位置 | HP 条上方 3 px | 水平居中于单位矩形 |

#### 新增战斗辅助方法

| 方法 | 说明 |
|------|------|
| `processBurningTick(alive)` | 遍历存活单位，对燃烧单位调用 `tickBurning()`；若单位因此死亡则调用 `board.removeUnit()` |

---

### 类间关系（UML 概要 v0.3）

```
Unit (abstract)
├── m_type: UnitType {Warrior, Mage, Support}
├── m_mana: int (0–MAX_MANA), MAX_MANA = 5
├── m_burning: bool, m_burningTurns: int
├── getAttackRange() / getAttackDamage() / getHealAmount() / canHeal()
├── gainMana() / resetMana() / getMana() / getMaxMana()
├── useSkill(board, allUnits) = 0       [纯虚：技能释放]
├── applyBurning(turns) / tickBurning() / isBurning()
├── heal(amount)    [不超过 m_maxHp，预留装备接口]
├── attack(target)  [基础伤害 + 装备加成]
│
├── Hero ──────────── WarriorHero / MageHero / SupportHero
└── Enemy ─────────── WarriorEnemy / MageEnemy / SupportEnemy

Board (8×8)  ◄──── Unit*[8][8]

Synera (QMainWindow)
├── Board + vector<unique_ptr<Unit>> + vector<PoolSlot>
├── 渲染: renderBoard / renderUnits / renderBenchPool / renderDragGhost / renderUI
├── 拖拽: processDragStart / processDrop (>50% 面积判定)
├── 战斗: processCombatTick / processBurningTick / findNearestEnemyFor / findHealTarget / moveStepToward
└── 键盘: R 重置
```

---

### 与 PA 阶段一 Checklist 的对应

| PA 要求 | 当前实现 |
|---------|---------|
| 8×8 棋盘 + 半场 | `Board` + `isPlayerHalf()` / `isEnemyHalf()` |
| 单位基类 Unit（HP/ATK 等） | `Unit` 基类 + `UnitType` 枚举 + `getAttackRange()` / `getAttackDamage()` 等纯虚接口 |
| 我方/敌方通过 owner 区分 | 派生类 `Hero`（A-前缀） / `Enemy`（E-前缀）区分，`dynamic_cast` 判定阵营 |
| 备战区与拖拽 | 左侧 Pool 面板 + 拖拽放置（>50% 面积吸附）+ 可拖回 Pool |
| 自动战斗 | `processCombatTick()` 全自动索敌/移动/攻击/治疗，~0.3s/tick |
| 战士/法师/辅助多态 | 6 个子类各重写 `getAttackRange()` / `getAttackDamage()` / `canHeal()` |
| 法力值与技能系统 | `gainMana()` / `resetMana()` 回蓝，`useSkill()` 纯虚多态技能，战士 80 伤害 / 法师 3×3 燃烧 / 辅助双目标治疗 30 |
| 燃烧 debuff | `applyBurning()` / `tickBurning()`，独立于施法者生命周期，10 dmg/tick × 4 回合 |
| GUI 展示棋盘、血条、法力点 | `renderBoard()` / `renderUnits()`（色块+标签+HP条+5法力点）/ `renderUI()`（图例+存活列表） |