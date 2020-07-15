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
    string remoteFileName = "pic.jpg";
    string localFileName = "C:/Users/zhb/Desktop/ftpclient/pic.jpg";
    std::ifstream ifs(localFileName, std::ios_base::in | std::ios_base::binary);

    FTPSession *se = new FTPSession();
    // UploadFileTask *task = nullptr;

    // connect
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

    // login
    QObject::connect(se, &FTPSession::loginSucceeded,
                     []() { qDebug("login succeeded"); });
    QObject::connect(se, &FTPSession::loginFailedWithMsg, [](string errorMsg) {
        qDebug() << "loginFailedWithMsg";
        qDebug() << "errorMsg: " << errorMsg.data();
    });
    QObject::connect(se, &FTPSession::loginFailed,
                     []() { qDebug("loginFailed"); });

    // getFilesize
    QObject::connect(se, &FTPSession::getFilesizeSucceeded,
                     [](int size) { qDebug() << "file size: " << size; });
    QObject::connect(se, &FTPSession::getFilesizeFailedWithMsg, [](string msg) {
        qDebug("getFilesizeFailedWithMsg");
        qDebug() << "msg: " << msg.data();
    });

    // getDir
    QObject::connect(se, &FTPSession::getDirSucceeded,
                     [](string dir) { qDebug() << "dir: " << dir.data(); });
    QObject::connect(se, &FTPSession::getDirFailedWithMsg, [](string msg) {
        qDebug("getDirFailedWithMsg");
        qDebug() << "msg: " << msg.data();
    });
    QObject::connect(se, &FTPSession::getDirFailed,
                     []() { qDebug("getDirFailed"); });

    // changeDir
    QObject::connect(se, &FTPSession::changeDirSucceeded,
                     []() { qDebug("changeDirSucceeded"); });
    QObject::connect(se, &FTPSession::changeDirFailedWithMsg, [](string msg) {
        qDebug("changeDirFailedWithMsg");
        qDebug() << "msg: " << msg.data();
    });
    QObject::connect(se, &FTPSession::changeDirFailed,
                     []() { qDebug("changeDirFailed"); });

    // setTransferMode
    QObject::connect(
        se, &FTPSession::setTransferModeSucceeded, [](bool binary) {
            qDebug() << "set transfer mode: " << (binary ? "binary" : "ascii");
        });
    QObject::connect(se, &FTPSession::setTransferModeFailedWithMsg,
                     [](string msg) {
                         qDebug("setTransferModeFailedWithMsg");
                         qDebug() << "msg: " << msg.data();
                     });
    QObject::connect(se, &FTPSession::setTransferModeFailed,
                     []() { qDebug("setTransferModeFailed"); });

    // deleteFile
    QObject::connect(se, &FTPSession::deleteFileSucceeded,
                     []() { qDebug("deleteFileSucceeded"); });
    QObject::connect(se, &FTPSession::deleteFileFailedWithMsg, [](string msg) {
        qDebug("deleteFileFailedWithMsg");
        qDebug() << "msg: " << msg.data();
    });
    QObject::connect(se, &FTPSession::deleteFileFailed,
                     []() { qDebug("deleteFileFailed"); });

    QObject::connect(se, &FTPSession::loginSucceeded, [&]() {
        //登录成功后设置二进制模式
        se->setTransferMode(true);
    });

    QObject::connect(se, &FTPSession::setTransferModeSucceeded, [&]() {
        //#ifndef DISABLED_CODE
        //下一步
        se->deleteFile("pic.jpg");
        //#endif

#ifndef DISABLED_CODE
        // 下一步上传文件
        task = new UploadFileTask(*se, remoteFileName, ifs); // new a task
        // connect signals for task
        // must be done after new a task, not before
        connectUploadSignals(task);
        task->start();
#endif
    });

    // test starts
    se->connect(test.hostname);
}
