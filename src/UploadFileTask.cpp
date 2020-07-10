#include "../include/UploadFileTask.h"
#include "../include/FTPFunction.h"
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QApplication>

namespace ftpclient
{

UploadFileTask::UploadFileTask(FTPSession &session, std::string fileName, std::ifstream &ifs)
    : session(session),
      remoteFileName(std::move(fileName)),
      ifs(ifs)
{
    QObject::connect(&session, &FTPSession::putPasvFailed,
                     [&](std::string errorMsg) { emit uploadFailedWithMsg(std::move(errorMsg)); });
    QObject::connect(&session, &FTPSession::sendFailed,
                     [&]() { emit uploadFailed(); });
    QObject::connect(&session, &FTPSession::recvFailed,
                     [&]() { emit uploadFailed(); });
    QObject::connect(&session, &FTPSession::putPasvSucceeded, this, &UploadFileTask::dataConnect);
}

void UploadFileTask::start()
{
    session.getPasvDataPort();
}

void UploadFileTask::dataConnect(int port)
{
    QFuture<ConnectToServerRes> connectRes = QtConcurrent::run(
                [&]() { return connectToServer(dataSock, session.getHostName(), std::to_string(port)); });
    while (!connectRes.isFinished())
        QApplication::processEvents();
    if (connectRes != ConnectToServerRes::SUCCEEDED)
        emit uploadFailed();

    startUploading();
}

void UploadFileTask::startUploading()
{
    emit uploadStarted();
    std::string errorMsg;
    QFuture<UploadToServerRes> future = QtConcurrent::run(
                [&]() { return uploadFileToServer(dataSock, remoteFileName, ifs, errorMsg); });
    while (!future.isFinished())
    {
        QApplication::processEvents();
        //TODO(zhb) 上传百分比
    }
    auto res = future.result();
    if (res == UploadToServerRes::SUCCEEDED)
        emit uploadSucceeded();
    else if (res == UploadToServerRes::FAILED_WITH_MSG)
        emit uploadFailedWithMsg(std::move(errorMsg));
    else
        emit uploadFailed();
}

}//namespace ftpclient
