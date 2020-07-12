#include "../include/mainwindow.h"
#include <QApplication>

void zhbtest();

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    zhbtest();

    return a.exec();
}
