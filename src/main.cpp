#include "../include/mainwindow.h"
#include <QApplication>

void test_z();

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    test_z();

    return a.exec();
}
