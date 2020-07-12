#include "../include/UploadFileTask.h"
#include "../include/FTPFunction.h"
#include <QApplication>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>

namespace ftpclient
{

    UploadFileTask::UploadFileTask(FTPSession &session, std::string fileName,
                                   std::ifstream &ifs)
        : session(session), remoteFileName(std::move(fileName)), ifs(ifs)
    {
        //在构造函数中连接信号槽，以便对切换 PASV 模式的各种结果做出响应
        //切换 PASV 模式失败时，发射 uploadFailedWithMsg 信号
        QObject::connect(&session, &FTPSession::putPasvFailedWithMsg,
                         [&](std::string errorMsg) {
                             emit uploadFailedWithMsg(std::move(errorMsg));
                         });
        // send()失败时，发射 uploadFailed 信号
        QObject::connect(&session, &FTPSession::sendFailed,
                         [&]() { emit uploadFailed(); });
        // recv()失败时，发射 uploadFailed 信号
        QObject::connect(&session, &FTPSession::recvFailed,
                         [&]() { emit uploadFailed(); });
        //切换 PASV 模式成功时，执行成员函数 dataConnect()，与服务器建立数据连接
        QObject::connect(&session, &FTPSession::putPasvSucceeded, this,
                         &UploadFileTask::dataConnect);
    }

    void UploadFileTask::start() { session.getPasvDataPort(); }

    void UploadFileTask::dataConnect(const std::string &hostname, int port)
    {
        //尝试与服务器建立连接
        QFuture<ConnectToServerRes> connectRes = QtConcurrent::run([&]() {
            return connectToServer(dataSock, hostname, std::to_string(port));
        });
        while (!connectRes.isFinished())
        {
            QApplication::processEvents();
        }

        //数据连接建立失败，发射 uploadFailed 信号
        if (connectRes.result() != ConnectToServerRes::SUCCEEDED)
            emit uploadFailed();
        //数据连接建立成功，发射 uploadStarted 信号
        //随后向服务器发送文件内容
        else
        {
            emit uploadStarted();
            startUploading();
        }
    }

    void UploadFileTask::startUploading()
    {
        std::string errorMsg;
        QFuture<UploadToServerRes> future = QtConcurrent::run([&]() {
            return uploadFileToServer(dataSock, remoteFileName, ifs, errorMsg);
        });
        while (!future.isFinished())
        {
            QApplication::processEvents();
            // TODO(zhb) 上传百分比
        }

        auto res = future.result();
        if (res == UploadToServerRes::SUCCEEDED)
            emit uploadSucceeded();
        else if (res == UploadToServerRes::FAILED_WITH_MSG)
            emit uploadFailedWithMsg(std::move(errorMsg));
        else
            emit uploadFailed();
    }

} // namespace ftpclient
