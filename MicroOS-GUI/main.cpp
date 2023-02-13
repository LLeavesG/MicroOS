#include "MicroOSGUI.h"
#include <QtWidgets/QApplication>
#include "kernel.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MicroOSGUI w;

    w.show();

    init_system();

    int ret = a.exec();
    return ret;
}
