#include "synera.h"
#include "ui_synera.h"
#include <QPainter>

Synera::Synera(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::Synera)
{
    // 关键！初始化UI，把Designer里的所有控件加载到窗口里
    ui->setupUi(this);

    // 窗口属性设置（和之前一致）
    setWindowTitle("Synera Project");
    resize(1200, 800);
}

void Synera::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    // 后续棋盘绘制写在这里
}