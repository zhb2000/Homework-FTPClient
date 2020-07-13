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

    UploadFileTask::~UploadFileTask()
    {
        // TODO(zhb) UploadFileTask 的析构函数
    }

    void UploadFileTask::start() { session.getPasvDataPort(); }

    void UploadFileTask::dataConnect(const std::string &hostname, int port)
    {
        //尝试与服务器建立连接
        QFuture<ConnectToServerRes> future = QtConcurrent::run([&]() {
            return connectToServer(dataSock, hostname, std::to_string(port));
        });
        while (!future.isFinished())
        {
            QApplication::processEvents();
        }

        //数据连接建立失败，发射 uploadFailed 信号
        if (future.result() != ConnectToServerRes::SUCCEEDED)
            emit uploadFailed();
        //数据连接建立成功，向服务器发 STOR 命令请求上传
        else
            uploadRequest();
    }

    void UploadFileTask::uploadRequest()
    {
        std::string errorMsg;
        QFuture<RequestToUpRes> future = QtConcurrent::run([&]() {
            return requestToUploadToServer(dataSock, remoteFileName, errorMsg);
        });
        while (!future.isFinished())
        {
            QApplication::processEvents();
        }

        auto res = future.result();
        if (res == RequestToUpRes::SUCCEEDED)
        {
            //服务器同意上传文件
            emit uploadStarted(); //发射 uploadStarted 信号
            uploadFileData();     //开始传输文件内容
        }
        else if (res == RequestToUpRes::FAILED_WITH_MSG)
            emit uploadFailedWithMsg(std::move(errorMsg));
        else // res == SEND_FAILED || res == RECV_FAILED
            emit uploadFailed();
    }

    void UploadFileTask::uploadFileData()
    {
        std::string errorMsg;
        QFuture<UploadFileDataRes> future = QtConcurrent::run([&]() {
            return uploadFileDataToServer(dataSock, remoteFileName, ifs,
                                          errorMsg);
        });
        while (!future.isFinished())
        {
            QApplication::processEvents();
            // TODO(zhb) 上传百分比
        }

        auto res = future.result();
        if (res == UploadFileDataRes::SUCCEEDED)
            emit uploadSucceeded();
        else if (res == UploadFileDataRes::SEND_FAILED)
            emit uploadFailed();
        else // res == READ_FILE_ERROR
            emit readFileError();
    }

} // namespace ftpclient
