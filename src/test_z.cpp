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

//关闭代码
#define DISABLED_CODE

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
    QObject::connect(task, &UploadFileTask::uploadFailed,
                     []() { qDebug("uploadFailed"); });
    QObject::connect(task, &UploadFileTask::readFileError,
                     []() { qDebug("readFileError"); });
    QObject::connect(task, &UploadFileTask::uploadPercentage,
                     [](int percentage) {
                         qDebug() << "percentage: " << percentage << "%";
                     });
}

void test_z()
{
    const auto &test = testcases[1];
    string remoteFileName = "file.txt";
    string localFileName = "C:/Users/zhb/Desktop/ftpclient/file.txt";
    std::ifstream ifs(localFileName, std::ios_base::in | std::ios_base::binary);

    FTPSession *se = new FTPSession();
    UploadFileTask *task = nullptr;

    QObject::connect(se, &FTPSession::connectionToServerSucceeded,
                     [&](std::string welcomeMsg) {
                         qDebug() << "welcomeMsg: " << welcomeMsg.c_str();
                         // 控制连接建立成功，下一步登录
                         se->login(test.username, test.password);
                     });
    QObject::connect(se, &FTPSession::recvFailed,
                     []() { qDebug() << "recvFailed"; });
    QObject::connect(se, &FTPSession::sendFailed,
                     []() { qDebug() << "sendFailed"; });

    QObject::connect(se, &FTPSession::createSocketFailed,
                     []() { qDebug() << "createSocketFailed"; });
    QObject::connect(se, &FTPSession::unableToConnectToServer,
                     []() { qDebug() << "unableToConnectToServer"; });

    QObject::connect(se, &FTPSession::loginSucceeded, [&]() {
        qDebug("login succeeded");
        //登录成功
#ifndef DISABLED_CODE
        //下一步获取文件大小
        se->getFilesize("text.txt");
#endif

        //#ifndef DISABLED_CODE
        // 下一步上传文件
        task = new UploadFileTask(*se, remoteFileName, ifs); // new a task
        // connect signals for task
        // must be done after new a task, not before
        connectUploadSignals(task);
        task->start();
        //#endif
    });
    QObject::connect(se, &FTPSession::loginFailedWithMsg, [](string errorMsg) {
        qDebug() << "loginFailedWithMsg";
        qDebug() << "errorMsg: " << errorMsg.data();
    });
    QObject::connect(se, &FTPSession::loginFailed,
                     []() { qDebug("loginFailed"); });
    QObject::connect(se, &FTPSession::getFilesizeSucceeded,
                     [](int size) { qDebug() << "file size: " << size; });
    QObject::connect(se, &FTPSession::getFilesizeFailedWithMsg, [](string msg) {
        qDebug("getFilesizeFailedWithMsg");
        qDebug() << "msg: " << msg.data();
    });

    // test starts
    se->connect(test.hostname);
}
