#ifndef SYNERA_H
#define SYNERA_H

#include <QMainWindow>
// 必须包含Qt自动生成的UI头文件，不然找不到控件
#include "ui_synera.h"

class Synera : public QMainWindow
{
    Q_OBJECT
public:
    explicit Synera(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    // UI指针，关联Designer里的所有控件（比如你加的start按钮）
    Ui::Synera *ui;
};

#endif // SYNERA_H