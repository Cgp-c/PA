#include "synera.h"
#include "ui_synera.h"
#include "hero.h"
#include "enemy.h"
#include "weapon.h"

#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFont>
#include <cmath>
#include <cstdlib>

Synera::Synera(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_selectedUnit(nullptr)
    , m_isPlayerTurn(true)
    , m_gameOver(false)
{
    ui->setupUi(this);
    setWindowTitle("Synera - 8x8 Battle Arena");
    resize(1200, 800);

    initGame();

    m_gameTimer = new QTimer(this);
    connect(m_gameTimer, &QTimer::timeout, this, &Synera::gameLoop);
    m_gameTimer->start(16);
    m_frameClock.start();
}

Synera::~Synera()
{
    delete ui;
}

// ─── 游戏初始化 ────────────────────────────────────────────────

void Synera::initGame()
{
    initUnits();
    m_selectedUnit = nullptr;
    m_isPlayerTurn = true;
    m_gameOver = false;
}

void Synera::initUnits()
{
    m_units.clear();
    m_board.clear();

    // 装备
    auto sword  = std::make_unique<Weapon>("Iron Sword", 15);
    auto axe    = std::make_unique<Weapon>("Battle Axe", 20);
    auto spear  = std::make_unique<Weapon>("Long Spear", 12);
    auto club   = std::make_unique<Weapon>("Wooden Club", 8);

    // 我方英雄（下半场 y ∈ [4,7]）
    auto h1 = std::make_unique<Hero>("Warrior",   100, 100, 2, 6);
    auto h2 = std::make_unique<Hero>("Archer",     80,  80, 5, 6);
    auto h3 = std::make_unique<Hero>("Mage",       70,  70, 3, 7);
    h1->setEquipment(sword.get());
    h2->setEquipment(spear.get());
    h3->setEquipment(nullptr);

    // 敌方单位（上半场 y ∈ [0,3]）
    auto e1 = std::make_unique<Enemy>("Goblin",    60,  60, 2, 1);
    auto e2 = std::make_unique<Enemy>("Orc",       90,  90, 5, 1);
    auto e3 = std::make_unique<Enemy>("Skeleton",  50,  50, 3, 2);
    e1->setEquipment(club.get());
    e2->setEquipment(axe.get());
    e3->setEquipment(nullptr);

    m_board.placeUnit(h1.get(), 2, 6);
    m_board.placeUnit(h2.get(), 5, 6);
    m_board.placeUnit(h3.get(), 3, 7);
    m_board.placeUnit(e1.get(), 2, 1);
    m_board.placeUnit(e2.get(), 5, 1);
    m_board.placeUnit(e3.get(), 3, 2);

    // 所有权转移
    m_units.push_back(std::move(h1));
    m_units.push_back(std::move(h2));
    m_units.push_back(std::move(h3));
    m_units.push_back(std::move(e1));
    m_units.push_back(std::move(e2));
    m_units.push_back(std::move(e3));

    m_weapons.push_back(std::move(sword));
    m_weapons.push_back(std::move(axe));
    m_weapons.push_back(std::move(spear));
    m_weapons.push_back(std::move(club));
}

// ─── 游戏主循环 ────────────────────────────────────────────────

void Synera::gameLoop()
{
    float dt = m_frameClock.elapsed() / 1000.0f;
    m_frameClock.restart();
    Q_UNUSED(dt);

    if (!m_isPlayerTurn && !m_gameOver) {
        processEnemyTurn();
    }

    update(); // 触发 paintEvent
}

// ─── 绘制 ──────────────────────────────────────────────────────

void Synera::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 背景
    painter.fillRect(rect(), QColor(30, 30, 40));

    renderBoard(painter);
    renderUnits(painter);
    renderUI(painter);
}

void Synera::renderBoard(QPainter& painter)
{
    for (int y = 0; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            QRect rc = cellRect(x, y);

            // 半场颜色
            QColor cellColor;
            if (m_board.isPlayerHalf(y))
                cellColor = QColor(40, 60, 90);   // 我方半场蓝灰
            else
                cellColor = QColor(80, 40, 40);   // 敌方半场暗红

            // 选中高亮
            if (m_selectedUnit) {
                Position sp = m_selectedUnit->getPosition();
                if (sp.x == x && sp.y == y)
                    cellColor = QColor(60, 120, 60);
            }

            painter.fillRect(rc, cellColor);
            painter.setPen(QPen(QColor(80, 80, 100), 1));
            painter.drawRect(rc);

            // 坐标标注
            painter.setPen(QColor(120, 120, 140));
            QFont smallFont;
            smallFont.setPixelSize(10);
            painter.setFont(smallFont);
            painter.drawText(rc.adjusted(2, 2, 0, 0),
                             QString("(%1,%2)").arg(x).arg(y));
        }
    }
}

void Synera::renderUnits(QPainter& painter)
{
    for (int y = 0; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            Unit* unit = m_board.getUnitAt(x, y);
            if (!unit || unit->isDisappeared())
                continue;

            QRect rc = cellRect(x, y);
            int margin = 6;
            QRect unitRect = rc.adjusted(margin, margin, -margin, -margin);

            // 颜色：我方蓝色，敌方红色
            bool isHero = dynamic_cast<Hero*>(unit) != nullptr;
            QColor fillColor = isHero ? QColor(60, 120, 220) : QColor(220, 60, 60);
            QColor borderColor = isHero ? QColor(100, 180, 255) : QColor(255, 100, 100);

            // 选中边框加粗
            if (unit == m_selectedUnit)
                borderColor = QColor(255, 255, 100);

            painter.setBrush(fillColor);
            painter.setPen(QPen(borderColor, unit == m_selectedUnit ? 3 : 1));
            painter.drawRoundedRect(unitRect, 6, 6);

            // 名字
            painter.setPen(Qt::white);
            QFont nameFont;
            nameFont.setPixelSize(12);
            nameFont.setBold(true);
            painter.setFont(nameFont);
            painter.drawText(unitRect.adjusted(4, 4, 0, 0),
                             QString::fromStdString(unit->getName()));

            // HP 条
            int barH = 8;
            int barY = unitRect.bottom() - barH - 4;
            double ratio = (double)unit->getHp() / unit->getMaxHp();
            QRect barBg(unitRect.left() + 4, barY, unitRect.width() - 8, barH);
            painter.setBrush(QColor(40, 40, 40));
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(barBg, 3, 3);

            QColor hpColor = ratio > 0.5 ? QColor(80, 220, 80)
                           : ratio > 0.25 ? QColor(220, 200, 40)
                           : QColor(220, 60, 60);
            QRect barFill(barBg.left(), barY,
                          static_cast<int>(barBg.width() * ratio), barH);
            painter.setBrush(hpColor);
            painter.drawRoundedRect(barFill, 3, 3);

            // HP 数字
            QFont hpFont;
            hpFont.setPixelSize(9);
            painter.setFont(hpFont);
            painter.setPen(Qt::white);
            QString hpText = QString("%1/%2").arg(unit->getHp()).arg(unit->getMaxHp());
            painter.drawText(barBg, Qt::AlignCenter, hpText);

            // 装备图标（如有）
            if (unit->getEquipment()) {
                painter.setPen(QColor(255, 215, 0));
                QFont eqFont;
                eqFont.setPixelSize(9);
                painter.setFont(eqFont);
                painter.drawText(unitRect.adjusted(4, 0, 0, 0),
                                 Qt::AlignBottom | Qt::AlignLeft,
                                 QString("[%1]")
                                     .arg(QString::fromStdString(
                                         unit->getEquipment()->getName())));
            }
        }
    }
}

void Synera::renderUI(QPainter& painter)
{
    QFont uiFont;
    uiFont.setPixelSize(16);
    uiFont.setBold(true);
    painter.setFont(uiFont);

    // 回合状态
    QString turnText;
    if (m_gameOver)
        turnText = "GAME OVER";
    else if (m_isPlayerTurn)
        turnText = "YOUR TURN - Click a hero to select";
    else
        turnText = "ENEMY TURN...";

    QColor turnColor = m_isPlayerTurn ? QColor(100, 200, 255) : QColor(255, 120, 120);
    painter.setPen(turnColor);

    int textX = BOARD_OFFSET_X + BOARD_PIXEL_SIZE + 30;
    painter.drawText(textX, BOARD_OFFSET_Y + 30, turnText);

    // 键位说明
    QFont helpFont;
    helpFont.setPixelSize(13);
    painter.setFont(helpFont);
    painter.setPen(QColor(160, 160, 180));

    int helpY = BOARD_OFFSET_Y + 80;
    painter.drawText(textX, helpY, "Controls:");
    painter.drawText(textX, helpY + 24, "- Click hero to select");
    painter.drawText(textX, helpY + 44, "- Click empty cell to move");
    painter.drawText(textX, helpY + 64, "- Click adjacent enemy to attack");
    painter.drawText(textX, helpY + 84, "- Press R to reset");

    // 单位列表
    int listY = helpY + 130;
    painter.setPen(QColor(200, 200, 220));
    painter.drawText(textX, listY, "Units:");

    listY += 24;
    for (auto& u : m_units) {
        // 只显示英雄/敌人（跳过武器对象）
        Hero* h = dynamic_cast<Hero*>(u.get());
        Enemy* e = dynamic_cast<Enemy*>(u.get());
        if (!h && !e) continue; // 跳过武器对象
        if (u->isDisappeared()) continue;

        painter.setPen(h ? QColor(100, 180, 255) : QColor(255, 120, 120));
        QString info = QString("%1  HP:%2/%3  (%4,%5)")
                           .arg(QString::fromStdString(u->getName()))
                           .arg(u->getHp())
                           .arg(u->getMaxHp())
                           .arg(u->getPosition().x)
                           .arg(u->getPosition().y);
        painter.drawText(textX + 10, listY, info);
        listY += 22;
    }
}

// ─── 坐标映射 ──────────────────────────────────────────────────

QRect Synera::cellRect(int x, int y) const
{
    return QRect(BOARD_OFFSET_X + x * CELL_SIZE,
                 BOARD_OFFSET_Y + y * CELL_SIZE,
                 CELL_SIZE, CELL_SIZE);
}

Unit* Synera::findUnitAt(int boardX, int boardY) const
{
    return m_board.getUnitAt(boardX, boardY);
}

// ─── 鼠标交互 ──────────────────────────────────────────────────

void Synera::mousePressEvent(QMouseEvent *event)
{
    if (m_gameOver || !m_isPlayerTurn)
        return;

    QPoint pos = event->pos();
    processPlayerClick(pos);
}

void Synera::processPlayerClick(const QPoint& mousePos)
{
    // 计算点击的棋盘坐标
    int gx = (mousePos.x() - BOARD_OFFSET_X) / CELL_SIZE;
    int gy = (mousePos.y() - BOARD_OFFSET_Y) / CELL_SIZE;

    if (!m_board.isValidPosition(gx, gy))
        return;

    Unit* clickedUnit = findUnitAt(gx, gy);

    // 情况1：没有选中任何单位 → 尝试选中我方单位
    if (!m_selectedUnit) {
        Hero* hero = dynamic_cast<Hero*>(clickedUnit);
        if (hero && !hero->isDisappeared()) {
            m_selectedUnit = hero;
        }
        return;
    }

    // 情况2：点击了自己的另一个英雄 → 切换选中
    Hero* clickedHero = dynamic_cast<Hero*>(clickedUnit);
    if (clickedHero && !clickedHero->isDisappeared()) {
        m_selectedUnit = clickedHero;
        return;
    }

    // 情况3：点击了相邻敌方单位 → 攻击
    Enemy* clickedEnemy = dynamic_cast<Enemy*>(clickedUnit);
    if (clickedEnemy && !clickedEnemy->isDisappeared()) {
        if (isAdjacent(m_selectedUnit->getPosition(), clickedEnemy->getPosition())) {
            m_selectedUnit->attack(*clickedEnemy);
            if (clickedEnemy->isDead())
                m_board.removeUnit(clickedEnemy->getPosition().x,
                                   clickedEnemy->getPosition().y);
            m_selectedUnit = nullptr;
            m_isPlayerTurn = false;
            return;
        }
    }

    // 情况4：点击了空格子且在我方半场 → 移动
    if (!clickedUnit && m_board.isPlayerHalf(gy)) {
        Position from = m_selectedUnit->getPosition();
        // 只能移动到相邻格
        if (isAdjacent(from, Position(gx, gy))) {
            Hero* hero = dynamic_cast<Hero*>(m_selectedUnit);
            m_board.removeUnit(from.x, from.y);
            m_board.placeUnit(hero, gx, gy);
            m_selectedUnit = nullptr;
            m_isPlayerTurn = false;
            return;
        }
    }

    // 情况5：点空 → 取消选中
    if (!clickedUnit) {
        m_selectedUnit = nullptr;
        return;
    }
}

// ─── 敌方回合 ──────────────────────────────────────────────────

void Synera::processEnemyTurn()
{
    // 收集存活的敌方单位
    std::vector<Enemy*> enemies;
    for (auto& u : m_units) {
        Enemy* e = dynamic_cast<Enemy*>(u.get());
        if (e && !e->isDead() && !e->isDisappeared())
            enemies.push_back(e);
    }

    for (Enemy* enemy : enemies) {
        Position epos = enemy->getPosition();
        Position nearest = findNearestHero(epos);

        if (nearest.x < 0) continue; // 没有英雄了

        // 相邻 → 攻击
        if (isAdjacent(epos, nearest)) {
            Unit* target = findUnitAt(nearest.x, nearest.y);
            if (target) {
                enemy->attack(*target);
                if (target->isDead()) {
                    m_board.removeUnit(nearest.x, nearest.y);
                }
            }
            continue;
        }

        // 否则向最近英雄移动一步
        int dx = (nearest.x > epos.x) ? 1 : (nearest.x < epos.x) ? -1 : 0;
        int dy = (nearest.y > epos.y) ? 1 : (nearest.y < epos.y) ? -1 : 0;
        int nx = epos.x + dx;
        int ny = epos.y + dy;

        if (m_board.isValidPosition(nx, ny) && !m_board.isOccupied(nx, ny)) {
            m_board.removeUnit(epos.x, epos.y);
            m_board.placeUnit(enemy, nx, ny);
        }
    }

    checkWinCondition();
    m_isPlayerTurn = true;
}

Position Synera::findNearestHero(const Position& from) const
{
    Position best(-1, -1);
    double bestDist = 1e9;
    for (auto& u : m_units) {
        Hero* h = dynamic_cast<Hero*>(u.get());
        if (!h || h->isDead() || h->isDisappeared())
            continue;
        Position hp = h->getPosition();
        double dist = std::sqrt((hp.x - from.x) * (hp.x - from.x) +
                                (hp.y - from.y) * (hp.y - from.y));
        if (dist < bestDist) {
            bestDist = dist;
            best = hp;
        }
    }
    return best;
}

bool Synera::isAdjacent(const Position& a, const Position& b) const
{
    int dx = std::abs(a.x - b.x);
    int dy = std::abs(a.y - b.y);
    return (dx + dy) == 1;
}

// ─── 胜负判定 ──────────────────────────────────────────────────

void Synera::checkWinCondition()
{
    bool hasHero = false;
    bool hasEnemy = false;
    for (auto& u : m_units) {
        if (u->isDisappeared() || u->isDead()) continue;
        if (dynamic_cast<Hero*>(u.get()))  hasHero = true;
        if (dynamic_cast<Enemy*>(u.get())) hasEnemy = true;
    }
    if (!hasHero || !hasEnemy)
        m_gameOver = true;
}

// ─── 按键事件 ──────────────────────────────────────────────────

void Synera::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_R) {
        initGame();
    }
    QMainWindow::keyPressEvent(event);
}
