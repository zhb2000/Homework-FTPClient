#include "../include/FTPSession.h"
#include "../include/FTPFunction.h"
#include "../include/MyUtils.h"
#include <QApplication>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <cstring>
#include <memory>

using std::unique_ptr;

namespace ftpclient
{

    FTPSession::FTPSession() : controlSock(INVALID_SOCKET), isConnected(false)
    {
    }

    FTPSession::~FTPSession()
    {
        if (controlSock != INVALID_SOCKET)
            closesocket(controlSock);
        WSACleanup();
    }

    void FTPSession::connect(const std::string &hostname, int port)
    {
        this->hostname = hostname;
        //连接并登录服务器
        QFuture<ConnectToServerRes> future = QtConcurrent::run([&]() {
            return connectToServer(controlSock, hostname, std::to_string(port),
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
            isConnected = true;

        //接收服务器端的一些欢迎信息
        std::string recvMsg;
        QFuture<int> recvRes = QtConcurrent::run(
            [&]() { return utils::recvAll(controlSock, recvMsg); });
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
            isConnected = false;
        }
    }

    void FTPSession::login(const std::string &username,
                           const std::string &password)
    {
        std::string errorMsg;
        QFuture<CmdToServerRet> future = QtConcurrent::run([&]() {
            return loginToServer(controlSock, username, password, errorMsg);
        });
        while (!future.isFinished())
            QApplication::processEvents();

        auto res = future.result();
        if (res == CmdToServerRet::SUCCEEDED)
            emit loginSucceeded();
        else if (res == CmdToServerRet::FAILED_WITH_MSG)
            emit loginFailedWithMsg(std::move(errorMsg));
        else
        {
            emit loginFailed();
            if (res == CmdToServerRet::SEND_FAILED)
                emit sendFailed();
            else // RECV_FAILED
                emit recvFailed();
        }
    }

    void FTPSession::getFilesize(const std::string &filename)
    {
        int filesize;
        std::string errorMsg;
        QFuture<CmdToServerRet> future = QtConcurrent::run([&]() {
            return getFilesizeOnServer(controlSock, filename, filesize,
                                       errorMsg);
        });
        while (!future.isFinished())
            QApplication::processEvents();

        auto res = future.result();
        if (res == CmdToServerRet::SUCCEEDED)
            emit getFilesizeSucceeded(filesize);
        else if (res == CmdToServerRet::FAILED_WITH_MSG)
            emit getFilesizeFailedWithMsg(std::move(errorMsg));
        else
        {
            emit getFilesizeFailed();
            if (res == CmdToServerRet::SEND_FAILED)
                emit sendFailed();
            else
                emit recvFailed();
        }
    }

    void FTPSession::getDir()
    {
        std::string dir;
        std::string errorMsg;
        QFuture<CmdToServerRet> future = QtConcurrent::run(
            [&]() { return getWorkingDirectory(controlSock, dir, errorMsg); });
        while (!future.isFinished())
            QApplication::processEvents();

        auto res = future.result();
        if (res == CmdToServerRet::SUCCEEDED)
            emit getDirSucceeded(std::move(dir));
        else if (res == CmdToServerRet::FAILED_WITH_MSG)
            emit getDirFailedWithMsg(std::move(errorMsg));
        else
        {
            emit getDirFailed();
            if (res == CmdToServerRet::SEND_FAILED)
                emit sendFailed();
            else
                emit recvFailed();
        }
    }

    void FTPSession::changeDir(const std::string &dir)
    {
        std::string errorMsg;
        QFuture<CmdToServerRet> future = QtConcurrent::run([&]() {
            return changeWorkingDirectory(controlSock, dir, errorMsg);
        });
        while (!future.isFinished())
            QApplication::processEvents();

        auto res = future.result();
        if (res == CmdToServerRet::SUCCEEDED)
            emit changeDirSucceeded();
        else if (res == CmdToServerRet::FAILED_WITH_MSG)
            emit changeDirFailedWithMsg(std::move(errorMsg));
        else
        {
            emit changeDirFailed();
            if (res == CmdToServerRet::SEND_FAILED)
                emit sendFailed();
            else
                emit recvFailed();
        }
    }

    void FTPSession::setTransferMode(bool binaryMode)
    {
        std::string errorMsg;
        QFuture<CmdToServerRet> future = QtConcurrent::run([&]() {
            return setBinaryOrAsciiTransferMode(controlSock, binaryMode,
                                                errorMsg);
        });
        while (!future.isFinished())
            QApplication::processEvents();

        auto res = future.result();
        if (res == CmdToServerRet::SUCCEEDED)
            emit setTransferModeSucceeded(binaryMode);
        else if (res == CmdToServerRet::FAILED_WITH_MSG)
            emit setTransferModeFailedWithMsg(std::move(errorMsg));
        else
        {
            emit setTransferModeFailed();
            if (res == CmdToServerRet::SEND_FAILED)
                emit sendFailed();
            else
                emit recvFailed();
        }
    }

    void FTPSession::close()
    {
        // TODO(zhb) 尚未完成
        if (this->isConnected)
            closesocket(controlSock);
        isConnected = false;
    }

} // namespace ftpclient
