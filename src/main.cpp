#include "../include/FTPSession.h"
#include "../include/mainwindow.h"

#include <QApplication>
#include <QtDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    using namespace ftpclient;
    FTPSession se;
    qDebug() << "before connect\n";
    qDebug() << "after connect\n";

    QObject::connect(&se, &FTPSession::connectionToServerSucceeded,
                     [&](std::string welcomeMsg) {
                         qDebug() << "welcomeMsg: " << welcomeMsg.c_str();
                         se.login("anonymous", "anonymous");
                     });
    QObject::connect(&se, &FTPSession::recvFailed,
                     []() { qDebug() << "recvFailed"; });
    QObject::connect(&se, &FTPSession::sendFailed,
                     []() { qDebug() << "sendFailed"; });
    QObject::connect(&se, &FTPSession::createSocketFailed,
                     []() { qDebug() << "createSocketFailed"; });
    QObject::connect(&se, &FTPSession::unableToConnectToServer,
                     []() { qDebug() << "unableToConnectToServer"; });
    QObject::connect(&se, &FTPSession::loginSucceeded,
                     []() { qDebug("login succeeded"); });
    QObject::connect(&se, &FTPSession::loginFailed, [](std::string errorMsg) {
        qDebug() << "errorMsg: " << errorMsg.data();
        qDebug() << "loginFailed\n";
    });

    se.connect("data.argo.org.cn");

    return a.exec();
}
