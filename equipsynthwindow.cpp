#include "equipsynthwindow.h"
#include <QFont>

EquipSynthWindow::EquipSynthWindow(QWidget* parent)
    : QWidget(parent, Qt::Window)
{
    setWindowTitle(QString::fromUtf8("\350\243\205\345\244\207\345\220\210\346\210\220\346\240\221")); // 装备合成树
    setFixedSize(560, 620);
    // 不设置 parent 为主窗口，保证独立
    setAttribute(Qt::WA_DeleteOnClose, false); // 关闭不销毁，手动管理生命周期
    setAttribute(Qt::WA_QuitOnClose, false);   // 关闭不退出程序
    initEntries();
}

EquipSynthWindow::~EquipSynthWindow() = default;

void EquipSynthWindow::closeEvent(QCloseEvent* event)
{
    event->ignore();          // 不真正关闭
    hide();                   // 隐藏窗口
    emit windowClosed();      // 通知主窗口
}

void EquipSynthWindow::initEntries()
{
    m_entries.clear();

    // ─── 攻击装 ──────────────────────────────────────
    m_entries.push_back({
        QString::fromUtf8("\347\254\246\346\226\207\345\244\247\345\211\221"),           // 符文大剑
        QString::fromUtf8("\351\255\224\346\263\225\347\237\263"),                       // 魔法石
        QString::fromUtf8("\345\211\221"),                                                 // 剑
        QString::fromUtf8("\346\224\273\345\207\273/\346\262\273\347\226\227+50\357\274\214\346\263\225\345\212\233\344\270\212\351\231\220-20"), // 攻击/治疗+50，法力上限-20
        QString::fromUtf8("\346\224\273\345\207\273\350\243\205"),                         // 攻击装
        QColor(210, 100, 30)
    });
    m_entries.push_back({
        QString::fromUtf8("\346\236\201\351\200\237\346\210\230\345\210\200"),           // 极速战刀
        QString::fromUtf8("\346\224\273\351\200\237"),                                     // 攻速
        QString::fromUtf8("\345\211\221"),                                                 // 剑
        QString::fromUtf8("\346\224\273\351\200\237\303\2270.5\357\274\214\346\224\273\345\207\273+30"), // 攻速×0.5，攻击+30
        QString::fromUtf8("\346\224\273\345\207\273\350\243\205"),                         // 攻击装
        QColor(210, 100, 30)
    });

    // ─── 防御装 ──────────────────────────────────────
    m_entries.push_back({
        QString::fromUtf8("\345\217\215\344\274\244\351\223\240\347\224\262"),           // 反伤铠甲
        QString::fromUtf8("\347\224\262"),                                                 // 甲
        QString::fromUtf8("\345\211\221"),                                                 // 剑
        QString::fromUtf8("HP+200\357\274\214\345\217\227\344\274\244\346\227\266\345\217\215\345\274\27150%\344\274\244\345\256\263"), // HP+200，受伤时反弹50%伤害
        QString::fromUtf8("\351\230\262\345\276\241\350\243\205"),                         // 防御装
        QColor(100, 140, 210)
    });
    m_entries.push_back({
        QString::fromUtf8("\347\224\237\345\221\275\351\207\215\347\224\262"),           // 生命重甲
        QString::fromUtf8("\347\224\262"),                                                 // 甲
        QString::fromUtf8("\347\224\262"),                                                 // 甲
        QString::fromUtf8("HP+400\357\274\214\351\230\262\345\276\241+20\357\274\210\345\207\217\344\274\244\357\274\211\357\274\214\345\217\227\346\262\273\347\226\227\303\2271.5"), // HP+400，防御+20，受治疗×1.5
        QString::fromUtf8("\351\230\262\345\276\241\350\243\205"),                         // 防御装
        QColor(100, 140, 210)
    });

    // ─── 攻速装 ──────────────────────────────────────
    m_entries.push_back({
        QString::fromUtf8("\347\226\276\351\243\216\346\211\213\345\245\227"),           // 疾风手套
        QString::fromUtf8("\346\224\273\351\200\237"),                                     // 攻速
        QString::fromUtf8("\351\255\224\346\263\225\347\237\263"),                       // 魔法石
        QString::fromUtf8("\346\224\273\351\200\237\303\2270.7\357\274\214\346\263\225\345\212\233\344\270\212\351\231\220+20"), // 攻速×0.7，法力上限+20
        QString::fromUtf8("\346\224\273\351\200\237\350\243\205"),                         // 攻速装
        QColor(180, 160, 40)
    });

    // ─── 蓝装 ────────────────────────────────────────
    m_entries.push_back({
        QString::fromUtf8("\345\244\215\346\264\273\347\237\263"),                       // 复活石
        QString::fromUtf8("\351\255\224\346\263\225\347\237\263"),                       // 魔法石
        QString::fromUtf8("\347\224\262"),                                                 // 甲
        QString::fromUtf8("HP+100\357\274\214\346\263\225\345\212\233\345\233\236\345\244\215\303\2272\357\274\214\351\230\265\344\272\241\346\227\266\346\273\241\350\241\200\345\244\215\346\264\273"), // HP+100，法力回复×2，阵亡时满血复活
        QString::fromUtf8("\350\223\235\350\243\205"),                                     // 蓝装
        QColor(130, 80, 210)
    });

    // ─── 攻击距离装 ──────────────────────────────────
    m_entries.push_back({
        QString::fromUtf8("\347\213\231\345\207\273\345\274\251"),                       // 狙击弩
        QString::fromUtf8("\346\210\230\351\251\254"),                                     // 战马
        QString::fromUtf8("\345\211\221"),                                                 // 剑
        QString::fromUtf8("\346\224\273\345\207\273\350\267\235\347\246\273+2\357\274\214\346\224\273\345\207\273+40"), // 攻击距离+2，攻击+40
        QString::fromUtf8("\345\260\204\347\250\213\350\243\205"),                         // 射程装
        QColor(80, 170, 140)
    });
}

void EquipSynthWindow::drawEntry(QPainter& painter, const SynthEntry& entry, int x, int y, int width)
{
    int cardH = 78;
    int margin = 6;

    // 卡片背景
    painter.setBrush(QColor(32, 32, 44));
    painter.setPen(QPen(QColor(55, 55, 70), 1));
    painter.drawRoundedRect(x, y, width, cardH, 6, 6);

    // 左侧类型色条
    QRect typeBar(x + 4, y + 4, 4, cardH - 8);
    painter.setBrush(entry.typeColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(typeBar, 2, 2);

    // 装备名称
    QFont nameFont;
    nameFont.setPixelSize(13);
    nameFont.setBold(true);
    painter.setFont(nameFont);
    painter.setPen(QColor(255, 210, 60));
    painter.drawText(x + 14, y + 16, entry.displayName);

    // 装备类型标签
    QFont typeFont;
    typeFont.setPixelSize(8);
    painter.setFont(typeFont);
    painter.setPen(entry.typeColor);
    painter.drawText(x + width - 56, y + 14, entry.equipType);

    // 合成公式
    QFont recipeFont;
    recipeFont.setPixelSize(10);
    recipeFont.setBold(true);
    painter.setFont(recipeFont);
    painter.setPen(QColor(200, 200, 220));
    QString recipe = entry.ingredient1 + "  +  " + entry.ingredient2;
    painter.drawText(x + 14, y + 40, recipe);

    // 箭头
    painter.setPen(QColor(255, 180, 40));
    QFont arrowFont;
    arrowFont.setPixelSize(10);
    painter.setFont(arrowFont);
    int arrowX = x + 14 + 120;
    painter.drawText(arrowX, y + 40, QString::fromUtf8("\342\206\222")); // →

    // 合成花费（箭头上方）
    QFont costFont;
    costFont.setPixelSize(8);
    painter.setFont(costFont);
    painter.setPen(QColor(180, 160, 100));
    painter.drawText(arrowX - 20, y + 24, QString::fromUtf8("\350\212\261\350\264\271300\351\207\221\345\270\201")); // 花费300金币

    // 效果描述
    QFont effectFont;
    effectFont.setPixelSize(9);
    painter.setFont(effectFont);
    painter.setPen(QColor(150, 210, 150));
    painter.drawText(x + 14, y + 62, entry.effects);
}

void EquipSynthWindow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor(22, 22, 32));

    // ── 标题 ──
    QFont titleFont;
    titleFont.setPixelSize(20);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(QColor(255, 210, 60));
    painter.drawText(20, 34, QString::fromUtf8("\350\243\205\345\244\207\345\220\210\346\210\220\346\240\221")); // 装备合成树

    // 副标题
    QFont subFont;
    subFont.setPixelSize(9);
    painter.setFont(subFont);
    painter.setPen(QColor(150, 150, 170));
    painter.drawText(20, 52, QString::fromUtf8("\346\211\200\346\234\211\351\253\230\347\272\247\350\243\205\345\244\207\345\220\210\346\210\220\346\226\271\346\263\225\345\217\212\346\225\210\346\236\234\357\274\210\346\257\217\346\254\241\345\220\210\346\210\220\346\266\210\350\200\227300\351\207\221\345\270\201\357\274\211")); // 所有高级装备合成方法及效果（每次合成消耗300金币）

    // ── 分割线 ──
    painter.setPen(QPen(QColor(60, 60, 80), 1));
    painter.drawLine(20, 64, width() - 20, 64);

    // ── 基础装备图例 ──
    int legendY = 78;
    QFont legendFont;
    legendFont.setPixelSize(9);
    legendFont.setBold(true);
    painter.setFont(legendFont);

    painter.setPen(QColor(180, 180, 200));
    painter.drawText(20, legendY, QString::fromUtf8("\345\237\272\347\241\200\350\243\205\345\244\207:")); // 基础装备:

    struct { QString name; QString icon; QString effect; QColor color; } legends[] = {
        {QString::fromUtf8("Iron Sword"), QString::fromUtf8("\345\211\221"), QString::fromUtf8("\346\224\273\345\207\273+30"), QColor(210, 100, 30)},            // 剑, 攻击+30
        {QString::fromUtf8("Chain Mail"), QString::fromUtf8("\347\224\262"), QString::fromUtf8("HP+150"), QColor(100, 140, 210)},                               // 甲, HP+150
        {QString::fromUtf8("Speed Gloves"), QString::fromUtf8("\346\224\273\351\200\237"), QString::fromUtf8("\346\224\273\351\200\237\303\2270.8"), QColor(180, 160, 40)}, // 攻速, 攻速×0.8
        {QString::fromUtf8("Blue Crystal"), QString::fromUtf8("\351\255\224\346\263\225\347\237\263"), QString::fromUtf8("\346\263\225\345\212\233\346\266\210\350\200\227\303\2270.5"), QColor(130, 80, 210)}, // 魔法石, 法力消耗×0.5
        {QString::fromUtf8("Warhorse"), QString::fromUtf8("\346\210\230\351\251\254"), QString::fromUtf8("\345\260\204\347\250\213+1"), QColor(80, 170, 140)}, // 战马, 射程+1
    };

    int lx = 110;
    for (auto& l : legends) {
        painter.setPen(l.color);
        QFont nameFont2;
        nameFont2.setPixelSize(8);
        nameFont2.setBold(true);
        painter.setFont(nameFont2);
        painter.drawText(lx, legendY - 2, l.icon);
        painter.setPen(QColor(140, 140, 160));
        QFont efFont2;
        efFont2.setPixelSize(7);
        painter.setFont(efFont2);
        painter.drawText(lx, legendY + 12, l.effect);
        lx += 80;
    }

    // ── 合成卡片 ──
    int cardStartY = legendY + 32;
    int cardW = (width() - 52) / 2; // 每行2个卡片
    int col1X = 20;
    int col2X = 20 + cardW + 12;
    int spacing = 8;

    for (int i = 0; i < (int)m_entries.size(); ++i) {
        int col = i % 2;
        int row = i / 2;
        int cx = (col == 0) ? col1X : col2X;
        int cy = cardStartY + row * (78 + spacing);
        drawEntry(painter, m_entries[i], cx, cy, cardW);
    }
}
