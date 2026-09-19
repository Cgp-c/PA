#ifndef POSTBATTLESTATSWINDOW_H
#define POSTBATTLESTATSWINDOW_H

#include <QWidget>
#include <QString>
#include <vector>

// 战后统计面板 —— 每场战斗结束后自动弹出的英雄战绩表。
// 数据为 POD 快照（不持有 Unit 指针，单位随后可能被回收/销毁）。
class PostBattleStatsWindow : public QWidget {
    Q_OBJECT

public:
    struct StatRow {
        QString name;
        int type = 0;   // UnitType 枚举值（小立绘块用）
        int star = 0;
        int dealt = 0;
        int taken = 0;
        int healed = 0;
        int kills = 0;
    };

    explicit PostBattleStatsWindow(QWidget* parent = nullptr);
    ~PostBattleStatsWindow() override;

    void setResults(const QString& title, const QString& subtitle,
                    const std::vector<StatRow>& rows);

protected:
    void paintEvent(QPaintEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    QString m_title;
    QString m_subtitle;
    std::vector<StatRow> m_rows;
};
#endif // POSTBATTLESTATSWINDOW_H
