#ifndef STARTSCREEN_H
#define STARTSCREEN_H

#include <QWidget>
#include <QRect>
#include <vector>

// 开始界面 —— 启动时的模式选择窗口（通关 / 无尽 / 自定义）。
// 自绘渲染层，选择后发射 modeSelected 并隐藏；R 键重置时重新弹出。
class StartScreen : public QWidget {
    Q_OBJECT

public:
    // 与 Synera::GameMode 的取值一一对应（int 传递避免头文件耦合）
    static constexpr int MODE_CAMPAIGN = 0;
    static constexpr int MODE_ENDLESS  = 1;
    static constexpr int MODE_CUSTOM   = 2;

    explicit StartScreen(QWidget* parent = nullptr);
    ~StartScreen() override;

    void refreshBestWave();   // 重读无尽最佳波数记录

signals:
    void modeSelected(int mode);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    std::vector<QRect> m_modeRects;
    int m_bestEndlessWave = 0;

    static int loadBestWave();
};
#endif // STARTSCREEN_H
