//测试用代码
// by zhb
#include "../include/FTPSession.h"
#include "../include/UploadFileTask.h"
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QtDebug>
#include <iostream>
#include <string>
#include <vector>

using namespace ftpclient;
using std::string;
using std::vector;

struct TestCase
{
    string hostname;
    string username;
    string password;
};

vector<TestCase> testcases = {{"data.argo.org.cn", "anonymous", "anonymous"},
                              {"localhost", "test01", "1111"}};

void connectUploadSignals(UploadFileTask *task);

void connectFTPSessionSignals(const TestCase &test,
                              const string &remoteFileName, std::ifstream &ifs,
                              FTPSession &se, UploadFileTask *&task)
{
    QObject::connect(&se, &FTPSession::connectionToServerSucceeded,
                     [&](std::string welcomeMsg) {
                         qDebug() << "welcomeMsg: " << welcomeMsg.c_str();
                         // go to login
                         se.login(test.username, test.password);
                     });
    QObject::connect(&se, &FTPSession::recvFailed,
                     []() { qDebug() << "recvFailed"; });
    QObject::connect(&se, &FTPSession::sendFailed,
                     []() { qDebug() << "sendFailed"; });

    QObject::connect(&se, &FTPSession::createSocketFailed,
                     []() { qDebug() << "createSocketFailed"; });
    QObject::connect(&se, &FTPSession::unableToConnectToServer,
                     []() { qDebug() << "unableToConnectToServer"; });

    QObject::connect(&se, &FTPSession::loginSucceeded, [&]() {
        qDebug("login succeeded");
        // go to upload file

        task = new UploadFileTask(se, remoteFileName, ifs); // new a task
        // connect signals for task
        // must be done after new a task, not before
        connectUploadSignals(task);
        task->start();
    });
    QObject::connect(&se, &FTPSession::loginFailedWithMsg, [](string errorMsg) {
        qDebug() << "loginFailedWithMsg";
        qDebug() << "errorMsg: " << errorMsg.data();
    });

    QObject::connect(
        &se, &FTPSession::putPasvSucceeded, [](string dataHost, int port) {
            qDebug("PASV succeeded");
            qDebug() << "host: " << dataHost.data() << "; port: " << port;
        });
    QObject::connect(&se, &FTPSession::putPasvFailedWithMsg,
                     [](string errorMsg) {
                         qDebug("PASV failed");
                         qDebug() << "msg: " << errorMsg.data();
                     });
}

void connectUploadSignals(UploadFileTask *task)
{
    QObject::connect(task, &UploadFileTask::uploadStarted,
                     []() { qDebug("uploadStarted"); });
    QObject::connect(task, &UploadFileTask::uploadSucceeded,
                     []() { qDebug("uploadSucceeded"); });
    QObject::connect(task, &UploadFileTask::uploadFailedWithMsg,
                     [](string errorMsg) {
                         qDebug("uploadFailedWithMsg");
                         qDebug() << errorMsg.data();
                     });
    QObject::connect(task, &UploadFileTask::uploadPercentage,
                     [](int percentage) {
                         qDebug() << "percentage: " << percentage << "%";
                     });
}

void test_z()
{
    const auto &test = testcases[0];
    string remoteFileName = "file.txt";
    string localFileName = "C:/Users/zhb/Desktop/ftpclient/file.txt";
    std::ifstream ifs(localFileName, std::ios_base::binary);

    FTPSession se;
    UploadFileTask *task = nullptr;

    // connect singnals and slots(for FTPSession)
    connectFTPSessionSignals(test, remoteFileName, ifs, se, task);

    // test starts: connect -> login -> new a task and init -> upload
    se.connect(test.hostname);
}
