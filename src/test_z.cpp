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

    FTPSession *se =
        new FTPSession(test.hostname, test.username, test.password);
    UploadFileTask *task = nullptr;

    QObject::connect(se, &FTPSession::recvFailed,
                     []() { qDebug() << "recvFailed"; });
    QObject::connect(se, &FTPSession::sendFailed,
                     []() { qDebug() << "sendFailed"; });

    // connect
    QObject::connect(se, &FTPSession::connectSucceeded,
                     [&](std::string welcomeMsg) {
                         qDebug() << "welcomeMsg: " << welcomeMsg.c_str();
                     });
    QObject::connect(se, &FTPSession::connectFailedWithMsg, [](string msg) {
        qDebug("connectFailedWithMsg");
        qDebug() << "msg: " << msg.data();
    });
    QObject::connect(se, &FTPSession::connectFailed,
                     []() { qDebug("connectFailed"); });
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

    // makeDir
    QObject::connect(se, &FTPSession::makeDirSucceeded,
                     []() { qDebug("makeDirSucceeded"); });
    QObject::connect(se, &FTPSession::makeDirFailedWithMsg, [](string msg) {
        qDebug("makeDirFailedWithMsg");
        qDebug() << "msg: " << msg.data();
    });
    QObject::connect(se, &FTPSession::makeDirFailed,
                     []() { qDebug("makeDirFailed"); });

    // removeDir
    QObject::connect(se, &FTPSession::removeDirSucceeded,
                     []() { qDebug("removeDirSucceeded"); });
    QObject::connect(se, &FTPSession::removeDirFailedWithMsg, [](string msg) {
        qDebug("removeDirFailedWithMsg");
        qDebug() << "msg: " << msg.data();
    });
    QObject::connect(se, &FTPSession::removeDirFailed,
                     []() { qDebug("removeDirFailed"); });

    // renameFile
    QObject::connect(se, &FTPSession::renameFileSucceeded,
                     []() { qDebug("renameFileSucceeded"); });
    QObject::connect(se, &FTPSession::renameFileFailedWithMsg, [](string msg) {
        qDebug("renameFileFailedWithMsg");
        qDebug() << "msg: " << msg.data();
    });
    QObject::connect(se, &FTPSession::renameFileFailed,
                     []() { qDebug("renameFileFailed"); });

    // listDir
    QObject::connect(se, &FTPSession::listDirSucceeded,
                     [](vector<string> dirList) {
                         qDebug("listDirSucceeded");
                         for (string &s : dirList)
                             qDebug() << s.data();
                     });
    QObject::connect(se, &FTPSession::listDirFailedWithMsg, [](string msg) {
        qDebug("listDirFailedWithMsg");
        qDebug() << "msg: " << msg.data();
    });
    QObject::connect(se, &FTPSession::listDirFailed,
                     []() { qDebug("listDirFailed"); });

    QObject::connect(se, &FTPSession::setTransferModeSucceeded, [&]() {
        //#ifndef DISABLED_CODE
        //下一步
        se->listWorkingDir();
        //#endif

#ifndef DISABLED_CODE
        // 下一步上传文件
        task = new UploadFileTask(*se, remoteFileName, ifs); // new a task
        // connect signals for task
        connectUploadSignals(task);
        task->start();
#endif
    });

    // test starts
    se->connectAndLogin();
}
