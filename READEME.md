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

---
## 当前代码架构（v0.5 — 刺客 + 帧计时器 + 关卡/经济/回收 + Bug修复/缩放调整）

基于 **Qt6 + C++17 + MinGW**，采用 **事件驱动游戏循环**（QTimer @ ~60fps），实现拖拽布阵与全自动战斗。

### 文件树

```text
PA/
├─ CMakeLists.txt
├─ README.md
├─ synera.ui                  # Qt Designer UI 布局
├─ main.cpp                   # 入口
├─ synera.h / synera.cpp      # 主窗口、渲染、游戏循环、帧级战斗、关卡/经济/回收
├─ unit.h / unit.cpp          # 单位抽象基类 + UnitType 枚举 + 速度/计时器 + 坐标/距离工具
├─ hero.h / hero.cpp          # Hero + WarriorHero / MageHero / SupportHero / AssassinHero
├─ enemy.h / enemy.cpp        # Enemy + WarriorEnemy / MageEnemy / SupportEnemy / AssassinEnemy
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

### 更新四：刺客职业 + 帧级独立计时器 + 关卡/经济/回收系统

**需求**：新增刺客职业，重构战斗系统为每单位独立帧计时，增加三关难度递增的关卡系统、金币商店、玩家生命值、以及战后回收机制。

---

#### 一、新增刺客职业 (`UnitType::Assassin`)

**Hero/Enemy 各一个子类**：`AssassinHero` / `AssassinEnemy`

| 属性 | 值 | 说明 |
|------|-----|------|
| HP | 40 | 低血量高伤害 |
| ATK | 50 | 基础攻击伤害 |
| Range | 1 | 近战范围（相邻4格） |
| 移速 | 30 帧/步 | 每 30 帧移动一次 |
| 攻速 | 40 帧/次 | 每 40 帧攻击一次 |
| 初始法力 | 5 (满值) | 开局即可释放技能 |

**刺客技能**：当与任意敌方单位的曼哈顿距离 ≤ 2 时触发——瞬移到目标**正下方**空位，造成 **80** 固定伤害。若正下方不可用，从目标的上下左右4方向中选择距离刺客当前位置最近的有效空位；等距时优先选择 y 较小的位置（上侧优先）。

**渲染**：刺客色块为金黄色（Hero `#C8B428` / Enemy `#A08C14`），中文标签 `"刺"`。

---

#### 二、帧级独立计时器系统（取代全局 TICK_INTERVAL）

原系统使用全局 `TICK_INTERVAL = 18` 帧，所有单位同步行动。新系统为**每个单位维护独立的移动和攻击计时器**。

**`Unit` 新增成员**（`unit.h/cpp`）：

| 成员 | 类型 | 说明 |
|------|------|------|
| `m_moveSpeed` | `int` | 移动间隔（帧），默认 30 |
| `m_attackSpeed` | `int` | 攻击间隔（帧），默认 60 |
| `m_moveTimer` | `int` | 移动计时器，每帧 +1，≥ m_moveSpeed 时可移动 |
| `m_attackTimer` | `int` | 攻击计时器，每帧 +1，≥ m_attackSpeed 时可攻击 |

**计时器规则**：

| 规则 | 说明 |
|------|------|
| 每帧递增 | 战斗阶段每帧对所有存活单位调用 `incrementTimers()` |
| 移速触发 | 当 `m_moveTimer ≥ m_moveSpeed` 时执行移动，然后 `resetMoveTimer()` |
| 攻速触发 | 当 `m_attackTimer ≥ m_attackSpeed` **且** 敌人在攻击范围内时执行攻击/治疗，然后 `resetAttackTimer()` |
| 离开范围重置 | 当没有目标在攻击范围内时，**每帧重置** `m_attackTimer = 0`，确保进入范围后需等待完整攻速间隔才发动首次攻击 |
| 独立并行 | 移动和攻击计时器完全独立，同一帧内可以同时移动和攻击 |

**各职业速度表**：

| 职业 | 移速 (帧/步) | 攻速 (帧/次) |
|------|:-----------:|:-----------:|
| Warrior | 30 | 60 |
| Mage | 30 | 60 |
| Support | 30 | 60 |
| Assassin | 30 | 40 |

**`Unit` 构造函数签名变更**：
```cpp
Unit(name, hp, maxHp, x, y, type, moveSpeed = 30, attackSpeed = 60, startMana = 0);
```
子类在构造函数中传入各自的速度参数，刺客额外传入 `startMana = MAX_MANA`。

**战斗循环变更**（`synera.cpp`）：
- `processCombatTick()` → `processCombatFrame()`，**每帧调用**（不再按 TICK_INTERVAL 跳帧）
- 新增 `m_frameCounter` 全局帧计数器
- 燃烧结算保持每 `BURNING_INTERVAL = 18` 帧执行一次

---

#### 三、三关关卡系统

**`Synera` 新增成员**：

| 成员 | 类型 | 初始值 | 说明 |
|------|------|--------|------|
| `m_currentLevel` | `int` | 1 | 当前关卡（1–3） |
| `MAX_LEVEL` | `static constexpr int` | 3 | 总关卡数 |
| `m_playerHp` | `int` | 100 | 玩家生命值 |
| `m_playerVictory` | `bool` | false | 是否通关胜利 |

**敌方配置**（`startBattle()`）：

| 关卡 | 战士 | 法师 | 辅助 | 刺客 | 总计 |
|:----:|:---:|:---:|:---:|:---:|:----:|
| 1 | 1 | 1 | 1 | 1 | 4 |
| 2 | 2 | 2 | 2 | 2 | 8 |
| 3 | 3 | 3 | 3 | 3 | 12 |

**通关条件**：通过全部 3 关即 **VICTORY**。

**失败扣血**：关卡失败时，棋盘敌方半场每剩余一个存活敌方单位，玩家扣除 **20 HP**。HP ≤ 0 即 **DEFEAT**。

**`endLevel(playerWon)` 流程**：

```
if 玩家获胜:
    回收场上存活英雄 → 回收槽（治疗20HP + 清空法力）
    全额获得 m_pendingGold
    if 当前关卡 == MAX_LEVEL → VICTORY
    else → m_currentLevel++, initLevel() 进入下一关准备阶段
else:
    统计敌方剩余数量 → m_playerHp -= 剩余数量 × 20
    获得 m_pendingGold 的 50%
    if m_playerHp <= 0 → DEFEAT
    else → m_currentLevel++, initLevel()
```

---

#### 四、金币系统与商店

**`Synera` 新增成员**：

| 成员 | 类型 | 初始值 | 说明 |
|------|------|--------|------|
| `m_gold` | `int` | 280 | 当前金币 |
| `m_pendingGold` | `int` | 0 | 本关战斗中累计的待结算金币 |

**购买价格**（`heroCost()`）：

| 职业 | 价格 (金) |
|------|:-------:|
| Warrior | 100 |
| Mage | 80 |
| Support | 50 |
| Assassin | 60 |

**击杀奖励**（`enemyGoldValue()`，通过 `m_pendingGold` 累计，回合结束后结算）：

| 敌方职业 | 奖励 (金) |
|----------|:-------:|
| Warrior | 80 |
| Mage | 60 |
| Support | 30 |
| Assassin | 30 |

**金币结算规则**：
- 玩家获胜：`m_gold += m_pendingGold`（100%）
- 玩家失败：`m_gold += m_pendingGold / 2`（50%）
- 敌方只剩辅助判胜时，剩余辅助也计入击杀奖励
- 燃烧造成的敌方死亡同样计入 `m_pendingGold`
- 金币**不在战斗中实时增加**，每关结束后统一结算

**商店 UI**（`renderShop()` 替代原 `renderBenchPool()`）：
- 仅在 Preparation 阶段显示，位于棋盘左侧 (x=10, y=80)
- 4 个槽位分别对应 Warrior / Mage / Support / Assassin
- 每个槽位显示：角色色块、名称、当前库存 `xN`、属性摘要、**购买按钮**（显示价格 `$XX`）
- 金币不足时按钮呈灰色且不可点击
- 准备阶段可通过拖拽将已购单位从商店池放到棋盘，也可拖回商店池返还库存

---

#### 五、回收系统

**`Synera` 新增成员**：

| 成员 | 类型 | 说明 |
|------|------|------|
| `m_recycleSlots` | `vector<Unit*>` | 8 个回收槽位（2行 × 4类型），存裸指针指向 `m_units` 中的英雄 |

**回收槽布局**（`renderRecycleSlots()`）：
- 位置：棋盘正下方（y = BOARD_OFFSET_Y + BOARD_PIXEL_SIZE + 25），水平居中
- 2 行 × 4 列（列对应：Warrior / Mage / Support / Assassin）
- 每个槽位 70×60 px，显示已回收英雄的色块 + 中文标签 + 微型 HP 条
- 空槽位显示暗色边框

**回收逻辑**（`endLevel()` 中）：
- 仅在**玩家获胜**时触发
- 遍历棋盘玩家半场，每个存活英雄：
  - `heal(20)`（不超过 maxHp）
  - `resetMana()` 清空技能条
  - `resetMoveTimer()` / `resetAttackTimer()` 重置计时器
  - 按类型放入对应列的回收槽（优先第 1 行，第 1 行满则放第 2 行）
- 若该类型两行均已满，该单位丢失

**交互**：
- 准备阶段可从回收槽拖拽英雄到棋盘
- 也可拖回回收槽空位（需类型匹配）
- `initLevel()` 时自动清理回收槽中已死亡/消失的单位

---

#### 六、GUI 更新

**顶部信息栏**（`renderUI()` 中新增）：

| 元素 | 位置 | 颜色 | 格式 |
|------|------|------|------|
| HP | 棋盘左上方 | 绿(>30) / 红(≤30) | `HP: 100` |
| 关卡 | 棋盘上方中央 | 金色 | `Level 1 / 3` |
| 金币 | 棋盘右上方 | 金色 | `Gold: 280` |

**阶段标题**：
- Preparation / Battle / **VICTORY! All levels cleared!** / **DEFEAT - GAME OVER**

**图例**：从 6 项扩展为 8 项（增加 Hero Assassin / Enemy Assassin）

---

#### 七、`GamePhase` 枚举变更

```cpp
// 旧
enum class GamePhase { Placement, Battle };

// 新
enum class GamePhase { Preparation, Battle };
```
`Preparation` = 购买英雄 + 布阵阶段（每关开始前）。

---

### 更新五：Bug 修复 + UI 缩放 20% + 基础通关金币 + 回收槽常驻

**需求**：修复敌方胜利不扣血、金币遗漏、回收槽为空等 bug；整体 UI 缩小 20%；新增每关基础金币 100；回收槽改为常驻透明显示。

---

#### 一、Bug 修复

| 问题 | 原因 | 修复 |
|------|------|------|
| 敌方胜利但玩家不扣血 | `endLevel(false)` 仅扫描棋盘上半区 (行0-3) 统计剩余敌人，但敌方单位可能已移动到玩家半场 | 改为扫描**整个棋盘** (`y=0..SIZE-1`)，并用 `dynamic_cast<Enemy*>` 精确统计 |
| 关卡失败但有剩余血量进入下一关时，未获得上回合的 50% 金币 | `m_pendingGold` 在技能击杀路径中未累计（技能直接调用 `takeDamage()`，绕过了 `processCombatFrame` 中的金币统计） | 技能释放前记录存活敌方列表，释放后对比，**新增死亡者计入 `m_pendingGold`** |
| 玩家获胜后回收槽始终为空 | `endLevel(true)` 仅扫描玩家半场 (行4-7) 回收英雄，但英雄可能追击到敌方半场 | 改为扫描**整个棋盘**回收存活英雄 |
| 关卡失败后敌方角色残留 | `endLevel(false)` 仅标记英雄消失，未处理敌方 | 新增遍历，将所有敌方单位 `setDisappeared(true)` |

#### 二、技能击杀金币追踪（`processCombatFrame`）

```cpp
if (u->getMana() >= Unit::MAX_MANA) {
    // 记录技能前存活敌方
    std::vector<Unit*> enemiesBeforeSkill;
    for (Unit* eu : alive) {
        if (!eu->isDead() && !eu->isDisappeared()
            && dynamic_cast<Enemy*>(eu) != nullptr)
            enemiesBeforeSkill.push_back(eu);
    }

    u->useSkill(m_board, alive);
    u->resetMana();
    u->resetMoveTimer();
    u->resetAttackTimer();

    // 技能击杀的敌方计入金币
    for (Unit* eu : enemiesBeforeSkill) {
        if (eu->isDead())
            m_pendingGold += enemyGoldValue(eu->getType());
    }
    continue;
}
```

---

#### 三、每关基础通关金币 100

在 `endLevel()` 中，无论胜负（只要玩家 HP > 0 可继续进入下一关），均额外获得 **100 基础金币**：

- 玩家胜利：`m_gold += m_pendingGold + 100`
- 玩家失败（HP > 0）：`m_gold += m_pendingGold / 2 + 100`

---

#### 四、关卡失败提示

新增 `m_showLevelLoss` 布尔标志（`synera.h`）：

- `endLevel(false)` 且 HP > 0 时设为 `true`
- `startBattle()` 时清零
- `initGame()` 时清零

**渲染**（`renderUI()`）：
- 准备阶段状态栏变红显示 `"PREPARATION PHASE  —  Level Failed!"`
- 棋盘中央叠加 36px 红色大字 `"Level Failed!"`

---

#### 五、整体 UI 缩放 ~20%

所有布局常量缩小约 20%（乘以 0.8）：

| 常量 | 旧值 | 新值 |
|------|:---:|:---:|
| `CELL_SIZE` | 80 | 64 |
| `BOARD_OFFSET_X` | 200 | 160 |
| `BOARD_OFFSET_Y` | 80 | 64 |
| `BOARD_PIXEL_SIZE` | 640 | 512 |
| `SHOP_Y` | 80 | 64 |
| `SHOP_SLOT_H` | 110 | 88 |
| `SHOP_WIDTH` | 170 | 136 |
| `RECYCLE_SLOT_W` | 70 | 56 |
| `RECYCLE_SLOT_H` | 60 | 48 |
| `RECYCLE_SPACING` | 10 | 8 |
| 窗口大小 | 1200×950 | 960×780 |

`RECYCLE_Y` 由公式 `BOARD_OFFSET_Y + BOARD_PIXEL_SIZE + 20` 自动计算（= 64 + 512 + 20 = 596）。

---

#### 六、回收槽常驻透明显示

回收槽**不再依赖任何阶段条件**，始终绘制在棋盘下方：

- **空槽**：`Qt::NoBrush`（透明填充）+ `QColor(255, 255, 255, 60)` 半透明白色边框
- **有单位**：实心色块 + 蓝色边框 + HP 微型条（同前）
- 标题 "Recycle" 常驻显示

---

### 类间关系（UML 概要 v0.5）

```
Unit (abstract)
├── m_type: UnitType {Warrior, Mage, Support, Assassin}
├── m_mana: int (0–MAX_MANA), MAX_MANA = 5
├── m_burning: bool, m_burningTurns: int
├── m_moveSpeed / m_attackSpeed / m_moveTimer / m_attackTimer
├── incrementTimers() / resetMoveTimer() / resetAttackTimer()
├── getAttackRange() / getAttackDamage() / getHealAmount() / canHeal()
├── gainMana() / resetMana() / getMana() / getMaxMana()
├── useSkill(board, allUnits) = 0       [纯虚：技能释放]
├── applyBurning(turns) / tickBurning() / isBurning()
├── heal(amount)    [不超过 m_maxHp]
├── attack(target)  [基础伤害 + 装备加成]
│
├── Hero ──────────── WarriorHero / MageHero / SupportHero / AssassinHero
└── Enemy ─────────── WarriorEnemy / MageEnemy / SupportEnemy / AssassinEnemy

Board (8×8)  ◄──── Unit*[8][8]

Synera (QMainWindow)
├── Board + vector<unique_ptr<Unit>> + vector<PoolSlot> (商店)
├── m_recycleSlots[8] (回收槽，常驻透明显示)
├── m_currentLevel / m_playerHp / m_gold / m_pendingGold
├── m_showLevelLoss (关卡失败覆盖提示)
├── m_frameCounter (全局帧计数)
├── 渲染: renderBoard / renderUnits / renderShop / renderRecycleSlots / renderDragGhost / renderUI
├── 拖拽: processDragStart / processDrop (支持商店池 + 回收槽 + 棋盘)
├── 战斗: processCombatFrame (每帧) / processBurningTick / findNearestEnemyFor / findHealTarget / moveStepToward
├── 结算: checkLevelEnd / endLevel (含100基础金币) / initLevel
└── 键盘: R 完全重置
```

---

### `Synera` 成员变量（v0.5 完整）

| 变量 | 类型 | 初始值 | 说明 |
|------|------|--------|------|
| `m_board` | `Board` | — | 8×8 棋盘数据 |
| `m_units` | `vector<unique_ptr<Unit>>` | — | 所有单位生命周期持有者 |
| `m_weapons` | `vector<unique_ptr<Weapon>>` | — | 所有装备生命周期持有者 |
| `m_shop` | `vector<PoolSlot>` | 4×0 | 商店库存（可购买数量） |
| `m_phase` | `GamePhase` | Preparation | Preparation / Battle |
| `m_gameOver` | `bool` | false | 游戏是否结束 |
| `m_playerVictory` | `bool` | false | 是否通关胜利 |
| `m_showLevelLoss` | `bool` | false | 关卡失败覆盖提示 |
| `m_frameCounter` | `int` | 0 | 全局帧计数器 |
| `m_currentLevel` | `int` | 1 | 当前关卡 (1–3) |
| `m_playerHp` | `int` | 100 | 玩家生命值 |
| `m_gold` | `int` | 280 | 当前金币 |
| `m_pendingGold` | `int` | 0 | 本关待结算金币 |
| `m_recycleSlots` | `vector<Unit*>` | 8×nullptr | 回收槽 (2行×4类型) |
| `m_draggedUnit` | `Unit*` | nullptr | 当前拖拽单位 |
| `m_dragCurrentPos` | `QPoint` | — | 拖拽中鼠标位置 |
| `m_dragFromShopIndex` | `int` | -1 | 从商店哪一槽拖出 (-2=回收槽, -1=棋盘) |
| `m_dragFromRecycleIndex` | `int` | -1 | 从回收槽哪一槽拖出 |

### `Synera` 常量（v0.5 完整）

| 常量 | 值 | 说明 |
|------|-----|------|
| `CELL_SIZE` | 64 | 每格像素 (~20%) |
| `BOARD_OFFSET_X/Y` | 160 / 64 | 棋盘左上角 (~20%) |
| `SHOP_X/Y` | 10 / 64 | 商店面板左上角 |
| `SHOP_SLOT_H` | 88 | 每个商店槽高度 (~20%) |
| `SHOP_WIDTH` | 136 | 商店面板宽度 (~20%) |
| `RECYCLE_Y` | 596 | 回收槽 Y 坐标 (= BOARD_OFFSET_Y + BOARD_PIXEL_SIZE + 20) |
| `RECYCLE_SLOT_W/H` | 56 / 48 | 回收槽尺寸 (~20%) |
| `RECYCLE_SPACING` | 8 | 回收槽间距 (~20%) |
| `BURNING_INTERVAL` | 18 | 燃烧结算帧间隔 |
| `MAX_LEVEL` | 3 | 最大关卡数 |

---

### 与 PA Checklist 的对应

| PA 要求 | 当前实现 |
|---------|---------|
| 8×8 棋盘 + 半场 | `Board` + `isPlayerHalf()` / `isEnemyHalf()` |
| 单位基类 Unit（HP/ATK 等） | `Unit` 基类 + `UnitType` 枚举 + `getAttackRange()` / `getAttackDamage()` 等纯虚接口 |
| 我方/敌方通过 owner 区分 | 派生类 `Hero`（A-前缀） / `Enemy`（E-前缀）区分，`dynamic_cast` 判定阵营 |
| 备战区与拖拽 | 左侧商店面板 + 回收槽 + 拖拽放置（>50% 面积吸附）+ 可拖回商店/回收槽 |
| 拖拽来源扩展 | 商店池 → 棋盘、回收槽 → 棋盘、棋盘 → 商店池/回收槽 |
| 自动战斗 | `processCombatFrame()` **每帧调用**，帧级独立计时器控制移动/攻击频率 |
| 四职业多态 | 8 个子类（Hero×4 + Enemy×4）各重写 `getAttackRange()` / `getAttackDamage()` / `canHeal()` / `useSkill()` |
| 法力值与技能系统 | `gainMana()` / `resetMana()` 回蓝，`useSkill()` 纯虚多态技能（战士/刺客=80单体，法师=3×3燃烧，辅助=双目标治疗30） |
| 燃烧 debuff | `applyBurning()` / `tickBurning()`，独立于施法者生命周期，10 dmg/tick × 4 回合 |
| GUI 展示 | `renderBoard()` / `renderUnits()` / `renderShop()` / `renderRecycleSlots()` / `renderUI()` |
| 关卡系统 | 3 关递增难度（敌方数量 4→8→12），通关胜利 / HP≤0 失败 |
| 金币经济 | 初始 280g，购买英雄（100/80/50/60），击杀奖励（80/60/30/30），关卡结束结算 |
| 玩家 HP | 初始 100，每关失败扣 20×剩余敌人数 |
| 回收系统 | 2行×4类型回收槽，获胜后回收英雄（治疗20 + 清空法力） |



### 待更新：法师技能燃烧帧过快，英雄合成，以及防止剩余角色>8的处理,...待修改：点击购买英雄后应当直接跟随鼠标移动而不是点击后再长按拖曳放置。关卡失败金钱获取问题？以及新的金币获取，每关固定100必得。
