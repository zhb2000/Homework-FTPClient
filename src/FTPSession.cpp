#include "../include/FTPSession.h"
#include "../include/FTPFunction.h"
#include "../include/MyUtils.h"
#include <QApplication>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <memory>

using std::unique_ptr;

namespace ftpclient
{

    FTPSession::FTPSession()
    {
        controlSock = INVALID_SOCKET;
        isConnectToServer = false;
    }

    FTPSession::~FTPSession()
    {
        if (controlSock != INVALID_SOCKET)
            closesocket(controlSock);
        WSACleanup();
    }

    void FTPSession::connect(const std::string &hostName, int port)
    {
        this->hostName = hostName;
        //连接并登录服务器
        QFuture<ConnectToServerRes> future = QtConcurrent::run([&]() {
            return connectToServer(controlSock, hostName, std::to_string(port));
        });
        while (!future.isFinished())
            QApplication::processEvents();
        if (future.result() != ConnectToServerRes::SUCCEEDED)
        {
            if (future.result() ==
                ConnectToServerRes::UNABLE_TO_CONNECT_TO_SERVER)
            {
                //无法连接到服务器
                emit unableToConnectToServer();
                return;
            }
            else
            {
                //创建socket失败
                emit createSocketFailed(future.result());
                return;
            }
        }
        else
            isConnectToServer = true;

        //接收服务器端的一些欢迎信息
        std::string recvMsg;
        QFuture<int> recvRes = QtConcurrent::run(
            [&]() { return utils::recv_all(controlSock, recvMsg); });
        while (!future.isFinished())
            QApplication::processEvents();
        if (recvRes.result() > 0)
        {
            //成功
            emit connectionToServerSucceeded(recvMsg);
        }
        else
        {
            // recv()失败
            emit recvFailed();
            isConnectToServer = false;
        }
    }

    void FTPSession::login(const std::string &userName,
                           const std::string &password)
    {
        std::string errorMsg;
        QFuture<LoginToServerRes> future = QtConcurrent::run([&]() {
            return loginToServer(controlSock, userName, password, errorMsg);
        });
        while (!future.isFinished())
            QApplication::processEvents();
        auto res = future.result();
        if (res == LoginToServerRes::SUCCEEDED)
            emit loginSucceeded();
        else if (res == LoginToServerRes::LOGIN_FAILED)
            emit loginFailedWithMsg(std::move(errorMsg));
        else if (res == LoginToServerRes::SEND_FAILED)
            emit sendFailed();
        else if (res == LoginToServerRes::RECV_FAILED)
            emit recvFailed();
    }

    void FTPSession::getPasvDataPort()
    {
        std::string dataHostname;
        int port;
        std::string errorMsg;
        QFuture<PutPasvModeRes> future = QtConcurrent::run([&]() {
            return putServerIntoPasvMode(controlSock, port, dataHostname,
                                         errorMsg);
        });
        while (!future.isFinished())
            QApplication::processEvents();
        if (future.result() == PutPasvModeRes::SUCCEEDED)
            emit putPasvSucceeded(std::move(dataHostname), port);
        else if (future.result() == PutPasvModeRes::FAILED)
            emit putPasvFailedWithMsg(std::move(errorMsg));
        else if (future.result() == PutPasvModeRes::SEND_FAILED)
            emit sendFailed();
        else if (future.result() == PutPasvModeRes::RECV_FAILED)
            emit recvFailed();
    }

    void FTPSession::close()
    {
        // TODO(zhb) 尚未完成
    }

} // namespace ftpclient
