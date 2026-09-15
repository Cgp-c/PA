#ifndef EQUIPSYNTHWINDOW_H
#define EQUIPSYNTHWINDOW_H

#include <QWidget>
#include <QPainter>
#include <QCloseEvent>
#include <vector>
#include <QString>

// 非模态装备合成树窗口 — 完全独立于主窗口的渲染和逻辑
class EquipSynthWindow : public QWidget {
    Q_OBJECT

public:
    explicit EquipSynthWindow(QWidget* parent = nullptr);
    ~EquipSynthWindow() override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

signals:
    void windowClosed();

private:
    struct SynthEntry {
        QString displayName;   // 展示名字（中文）
        QString iconFile;      // 结果装备图标文件名（:/equip/ 下）
        QString ingIcon1;      // 材料1图标文件名
        QString ingIcon2;      // 材料2图标文件名
        QString effects;       // 效果描述
        QString equipType;     // 装备类型标签
        QColor  typeColor;     // 类型颜色
    };
    std::vector<SynthEntry> m_entries;

    void initEntries();
    void drawEntry(QPainter& painter, const SynthEntry& entry, int x, int y, int width);
};

#endif // EQUIPSYNTHWINDOW_H
