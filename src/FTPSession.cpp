#include "../include/FTPSession.h"
#include "../include/FTPFunction.h"
#include "../include/ListTask.h"
#include "../include/MyUtils.h"
#include "../include/RunAsyncAwait.h"
#include <QApplication>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <cstring>
#include <memory>

namespace ftpclient
{

    const int FTPSession::SOCKET_SEND_TIMEOUT;
    const int FTPSession::SOCKET_RECV_TIMEOUT;
    const int FTPSession::SEND_NOOP_TIME;

    FTPSession::FTPSession(const std::string &hostname,
                           const std::string &username,
                           const std::string password, int port,
                           bool autoKeepAlive)
        : hostname(hostname),
          port(port),
          username(username),
          password(password),
          controlSock(INVALID_SOCKET),
          isConnected(false)
    {
        //连接到服务器后，就登录进去
        QObject::connect(this, &FTPSession::connectSucceeded,
                         [this]() { this->login(); });
        //登录成功
        QObject::connect(this, &FTPSession::loginSucceeded, [this]() {
            //切换成二进制传输模式
            this->setTransferMode(true);
            //开始间歇性发送 NOOP 命令以保持连接
        });
        if (autoKeepAlive)
        {
            QObject::connect(&sendNoopTimer, &QTimer::timeout, this,
                             &FTPSession::sendNoop);
            sendNoopTimer.start(SEND_NOOP_TIME);
        }
    }

    void FTPSession::sendNoop()
    {
        utils::asyncAwait([this]() {
            if (sockMutex.try_lock())
            {
                std::string noopCmd = "NOOP\r\n";
                std::string recvMsg;
                send(controlSock, noopCmd.c_str(), noopCmd.length(), 0);
                //正常为"200 OK"
                //为了省事，这里不校验返回码
                utils::recvFtpMsg(controlSock, recvMsg);
                sockMutex.unlock();
            }
            else //此时别的线程正在发命令，因此无需用 NOOP 保活
                return;
        });
        sendNoopTimer.start(SEND_NOOP_TIME);
    }

    void FTPSession::connectAndLogin() { this->connect(); }

    void FTPSession::connect()
    {
        //连接服务器
        auto connectRes = utils::asyncAwait<ConnectToServerRes>([this]() {
            sockMutex.lock();
            auto result =
                connectToServer(controlSock, hostname, std::to_string(port),
                                SOCKET_SEND_TIMEOUT, SOCKET_RECV_TIMEOUT);
            sockMutex.unlock();
            return result;
        });
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
        auto res = utils::asyncAwait<RecvMultRes>([this, &recvMsg]() {
            sockMutex.lock();
            auto result = recvWelcomMsg(controlSock, recvMsg);
            sockMutex.unlock();
            return result;
        });
        if (res == RecvMultRes::SUCCEEDED)
            emit connectSucceeded(std::move(recvMsg));
        else if (res == RecvMultRes::FAILED_WITH_MSG)
        {
            isConnected = false;
            emit connectFailedWithMsg(std::move(recvMsg));
        }
        else // res == FAILED
        {
            isConnected = false;
            emit connectFailed();
        }
    }

    void FTPSession::login()
    {
        runProcedure(
            [this](std::string &msg) {
                sockMutex.lock();
                auto result =
                    loginToServer(controlSock, username, password, msg);
                sockMutex.unlock();
                return result;
            },
            &FTPSession::loginSucceeded, &FTPSession::loginFailedWithMsg,
            &FTPSession::loginFailed);
    }

    void FTPSession::getFilesize(const std::string &filename)
    {
        int filesize;
        std::string errorMsg;
        auto res = utils::asyncAwait<CmdToServerRet>([&]() {
            sockMutex.lock();
            auto result =
                getFilesizeOnServer(controlSock, filename, filesize, errorMsg);
            sockMutex.unlock();
            return result;
        });
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
        auto res = utils::asyncAwait<CmdToServerRet>([&]() {
            sockMutex.lock();
            auto result = getWorkingDirectory(controlSock, dir, errorMsg);
            sockMutex.unlock();
            return result;
        });
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
            [&dir, this](std::string &msg) {
                sockMutex.lock();
                auto result = changeWorkingDirectory(controlSock, dir, msg);
                sockMutex.unlock();
                return result;
            },
            &FTPSession::changeDirSucceeded,
            &FTPSession::changeDirFailedWithMsg, &FTPSession::changeDirFailed);
    }

    void FTPSession::setTransferMode(bool binaryMode)
    {
        std::string errorMsg;
        auto res = utils::asyncAwait<CmdToServerRet>([&]() {
            sockMutex.lock();
            auto result =
                setBinaryOrAsciiTransferMode(controlSock, binaryMode, errorMsg);
            sockMutex.unlock();
            return result;
        });
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
            [&filename, this](std::string &msg) {
                sockMutex.lock();
                auto result = deleteFileOnServer(controlSock, filename, msg);
                sockMutex.unlock();
                return result;
            },
            &FTPSession::deleteFileSucceeded,
            &FTPSession::deleteFileFailedWithMsg,
            &FTPSession::deleteFileFailed);
    }

    void FTPSession::makeDir(const std::string &dir)
    {
        runProcedure(
            [&dir, this](std::string &msg) {
                sockMutex.lock();
                auto result = makeDirectoryOnServer(controlSock, dir, msg);
                sockMutex.unlock();
                return result;
            },
            &FTPSession::makeDirSucceeded, &FTPSession::makeDirFailedWithMsg,
            &FTPSession::makeDirFailed);
    }

    void FTPSession::removeDir(const std::string &dir)
    {
        runProcedure(
            [&dir, this](std::string &msg) {
                sockMutex.lock();
                auto result = removeDirectoryOnServer(controlSock, dir, msg);
                sockMutex.unlock();
                return result;
            },
            &FTPSession::removeDirSucceeded,
            &FTPSession::removeDirFailedWithMsg, &FTPSession::removeDirFailed);
    }

    void FTPSession::renameFile(const std::string &oldName,
                                const std::string &newName)
    {
        runProcedure(
            [&](std::string &msg) {
                sockMutex.lock();
                auto result =
                    renameFileOnServer(controlSock, oldName, newName, msg);
                sockMutex.unlock();
                return result;
            },
            &FTPSession::renameFileSucceeded,
            &FTPSession::renameFileFailedWithMsg,
            &FTPSession::renameFileFailed);
    }

    void FTPSession::listWorkingDir(bool isNameList)
    {
        std::string errorMsg;
        std::vector<std::string> listStrings;
        auto res = utils::asyncAwait<ListTask::Res>([&]() {
            sockMutex.lock();
            ListTask task(*this, ".", isNameList);
            auto result = task.getListStrings(listStrings, errorMsg);
            sockMutex.unlock();
            return result;
        });
        if (res == ListTask::Res::SUCCEEDED)
            emit listDirSucceeded(std::move(listStrings));
        else if (res == ListTask::Res::FAILED_WITH_MSG)
            emit listDirFailedWithMsg(std::move(errorMsg));
        else
            emit listDirFailed();
    }

    void FTPSession::quit()
    {
        // 把控制连接关闭
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
