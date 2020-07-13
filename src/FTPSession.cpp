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
        this->hostname = hostName;
        //连接并登录服务器
        QFuture<ConnectToServerRes> future = QtConcurrent::run([&]() {
            return connectToServer(controlSock, hostName, std::to_string(port),
                                   FTPSession::SOCKET_SEND_TIMEOUT,
                                   FTPSession::SOCKET_RECV_TIMEOUT);
        });
        while (!future.isFinished())
        {
            QApplication::processEvents();
        }

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
        {
            QApplication::processEvents();
        }

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
        else if (res == LoginToServerRes::FAILED_WITH_MSG)
            emit loginFailedWithMsg(std::move(errorMsg));
        else if (res == LoginToServerRes::SEND_FAILED)
            emit sendFailed();
        else if (res == LoginToServerRes::RECV_FAILED)
            emit recvFailed();
    }

    void FTPSession::usePassiveMode()
    {
        std::string dataHostname;
        int port;
        std::string errorMsg;
        QFuture<PutPasvModeRes> future1 = QtConcurrent::run([&]() {
            return putServerIntoPasvMode(controlSock, port, dataHostname,
                                         errorMsg);
        });
        while (!future1.isFinished())
        {
            QApplication::processEvents();
        }

        auto res1 = future1.result();
        if (res1 == PutPasvModeRes::SUCCEEDED)
            emit putPassiveSucceeded(std::move(dataHostname), port);
        else if (res1 == PutPasvModeRes::FAILED_WITH_MSG)
            emit putPassiveFailedWithMsg(std::move(errorMsg));
        else if (res1 == PutPasvModeRes::SEND_FAILED)
            emit sendFailed();
        else if (res1 == PutPasvModeRes::RECV_FAILED)
            emit recvFailed();
        else // res1 == HAVE_TO_USE_EPSV
        {
            QFuture<PutEpsvModeRes> future2 = QtConcurrent::run([&]() {
                return putServerIntoEpsvMode(controlSock, port, errorMsg);
            });
            while (!future2.isFinished())
            {
                QApplication::processEvents();
            }

            auto res2 = future2.result();
            if (res2 == PutEpsvModeRes::SUCCEEDED)
                emit putPassiveSucceeded(this->getHostName(), port);
            else if (res2 == PutEpsvModeRes::FAILED_WITH_MSG)
                emit emit putPassiveFailedWithMsg(std::move(errorMsg));
            else if (res2 == PutEpsvModeRes::SEND_FAILED)
                emit sendFailed();
            else // res2 == PutEpsvModeRes::RECV_FAILED
                emit recvFailed();
        }
    }

    void FTPSession::close()
    {
        // TODO(zhb) 尚未完成
        closesocket(controlSock);
    }

} // namespace ftpclient
