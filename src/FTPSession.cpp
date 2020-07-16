#include "../include/FTPSession.h"
#include "../include/FTPFunction.h"
#include "../include/MyUtils.h"
#include "../include/RunAsyncAwait.h"
#include <QApplication>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <cstring>
#include <memory>

using std::unique_ptr;

namespace ftpclient
{

    const int FTPSession::SOCKET_SEND_TIMEOUT;
    const int FTPSession::SOCKET_RECV_TIMEOUT;

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
        auto connectRes = utils::asyncAwait<ConnectToServerRes>(
            connectToServer, controlSock, hostname, std::to_string(port),
            FTPSession::SOCKET_SEND_TIMEOUT, FTPSession::SOCKET_RECV_TIMEOUT);
        if (connectRes != ConnectToServerRes::SUCCEEDED)
        {
            if (connectRes == ConnectToServerRes::UNABLE_TO_CONNECT_TO_SERVER)
            {
                emit unableToConnectToServer(); //无法连接到服务器
                return;
            }
            else
            {
                emit createSocketFailed(connectRes); //创建socket失败
                return;
            }
        }
        else
            isConnected = true;

        //接收服务器端的一些欢迎信息
        std::string recvMsg;
        int recvSize =
            utils::asyncAwait<int>(utils::recvAll, controlSock, recvMsg);
        if (recvSize > 0)
            emit connectionToServerSucceeded(recvMsg);
        else
        {
            emit unableToConnectToServer();
            emit recvFailed();
            isConnected = false;
        }
    }

    void FTPSession::login(const std::string &username,
                           const std::string &password)
    {
        runProcedure(
            [&](std::string &msg) {
                return loginToServer(controlSock, username, password, msg);
            },
            &FTPSession::loginSucceeded, &FTPSession::loginFailedWithMsg,
            &FTPSession::loginFailed);
    }

    void FTPSession::getFilesize(const std::string &filename)
    {
        int filesize;
        std::string errorMsg;
        auto res = utils::asyncAwait<CmdToServerRet>(
            getFilesizeOnServer, controlSock, filename, filesize, errorMsg);
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
        auto res = utils::asyncAwait<CmdToServerRet>(
            getWorkingDirectory, controlSock, dir, errorMsg);
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
        runProcedure(
            [&](std::string &msg) {
                return changeWorkingDirectory(controlSock, dir, msg);
            },
            &FTPSession::changeDirSucceeded,
            &FTPSession::changeDirFailedWithMsg, &FTPSession::changeDirFailed);
    }

    void FTPSession::setTransferMode(bool binaryMode)
    {
        std::string errorMsg;
        auto res = utils::asyncAwait<CmdToServerRet>(
            setBinaryOrAsciiTransferMode, controlSock, binaryMode, errorMsg);
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

    void FTPSession::deleteFile(const std::string &filename)
    {
        runProcedure(
            [&](std::string &msg) {
                return deleteFileOnServer(controlSock, filename, msg);
            },
            &FTPSession::deleteFileSucceeded,
            &FTPSession::deleteFileFailedWithMsg,
            &FTPSession::deleteFileFailed);
    }

    void FTPSession::makeDir(const std::string &dir)
    {
        runProcedure(
            [&](std::string &msg) {
                return makeDirectoryOnServer(controlSock, dir, msg);
            },
            &FTPSession::makeDirSucceeded, &FTPSession::makeDirFailedWithMsg,
            &FTPSession::makeDirFailed);
    }

    void FTPSession::removeDir(const std::string &dir)
    {
        runProcedure(
            [&](std::string &msg) {
                return removeDirectoryOnServer(controlSock, dir, msg);
            },
            &FTPSession::removeDirSucceeded,
            &FTPSession::removeDirFailedWithMsg, &FTPSession::removeDirFailed);
    }

    void FTPSession::close()
    {
        // TODO(zhb) 尚未完成
        if (this->isConnected)
            closesocket(controlSock);
        isConnected = false;
    }

    void FTPSession::runProcedure(
        std::function<CmdToServerRet(std::string &)> func,
        void (FTPSession::*succeededSignal)(),
        void (FTPSession::*failedWithMsgSignal)(std::string),
        void (FTPSession::*failedSignal)())
    {
        std::string errorMsg;
        auto res = utils::asyncAwait<CmdToServerRet>(func, errorMsg);
        if (res == CmdToServerRet::SUCCEEDED)
            emit(this->*succeededSignal)();
        else if (res == CmdToServerRet::FAILED_WITH_MSG)
            emit(this->*failedWithMsgSignal)(std::move(errorMsg));
        else
        {
            emit(this->*failedSignal)();
            if (res == CmdToServerRet::SEND_FAILED)
                emit sendFailed();
            else
                emit recvFailed();
        }
    }

} // namespace ftpclient
