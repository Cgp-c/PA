#include <QApplication>
#include "synera.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Synera mainWin;
    mainWin.show();
    return app.exec();
}