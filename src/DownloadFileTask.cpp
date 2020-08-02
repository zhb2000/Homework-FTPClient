#include "../include/DownloadFileTask.h"
#include "../include/FTPFunction.h"
#include "../include/MyUtils.h"
#include "../include/RunAsyncAwait.h"
#include <QApplication>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <algorithm>
#include <fstream>
#include <io.h>
#include <iostream>
#include <regex>

namespace ftpclient
{

    const int DownloadFileTask::SENDTIMEOUT;
    const int DownloadFileTask::RECVTIMEOUT;

    DownloadFileTask::DownloadFileTask(FTPSession &session,
                                       const std::string &localFilepath,
                                       const std::string &remoteFilepath)
        : session(session.getHostname(), session.getUsername(),
                  session.getPassword(), session.getPort(), false),
          localFilepath(localFilepath),
          remoteFilepath(remoteFilepath),
          ofs(localFilepath, std::ios_base::out | std::ios_base::binary),
          dataSocket(INVALID_SOCKET),
          isDataConnected(false),
          isSetStop(false),
          isReset(false)
    {
        this->connectSignals();
    }

    DownloadFileTask::~DownloadFileTask()
    {
        this->quit();
        WSACleanup();
    }

    void DownloadFileTask::connectSignals()
    {
        //控制连接建立失败，发射错误信号
        QObject::connect(&session, &FTPSession::connectFailedWithMsg,
                         [this](std::string msg) {
                             emit downloadFailedWithMsg(std::move(msg));
                         });
        QObject::connect(&session, &FTPSession::connectFailed,
                         [this]() { emit downloadFailed(); });
        QObject::connect(&session, &FTPSession::createSocketFailed,
                         [this]() { emit downloadFailed(); });
        QObject::connect(&session, &FTPSession::unableToConnectToServer,
                         [this]() { emit downloadFailed(); });
        //登录失败，发射故障信号
        QObject::connect(&session, &FTPSession::loginFailedWithMsg,
                         [this](std::string msg) {
                             emit downloadFailedWithMsg(std::move(msg));
                         });
        QObject::connect(&session, &FTPSession::loginFailed,
                         [this]() { emit downloadFailed(); });
        QObject::connect(&session, &FTPSession::loginFailedWithMsg,
                         [this](std::string msg) {
                             emit downloadFailedWithMsg(std::move(msg));
                         });
        //传输模式设置失败，发射故障信号
        QObject::connect(&session, &FTPSession::setTransferModeFailed,
                         [this]() { emit downloadFailed(); });
        QObject::connect(&session, &FTPSession::setTransferModeFailed,
                         [this]() { emit downloadFailed(); });
        //传输模式设置成功，下一步获取服务器上文件的大小
        QObject::connect(&session, &FTPSession::setTransferModeSucceeded,
                         [this]() { session.getFilesize(remoteFilepath); });
        //获取文件大小失败，发送故障信号
        QObject::connect(&session, &FTPSession::getFilesizeFailedWithMsg,
                         [this](std::string msg) {
                             emit downloadFailedWithMsg(std::move(msg));
                         });
        QObject::connect(&session, &FTPSession::getFilesizeFailed,
                         [this]() { emit downloadFailed(); });
        //获取文件大小成功，按照既定流程执行
        QObject::connect(&session, &FTPSession::getFilesizeSucceeded,
                         [this](long long filesize) {
                             this->remoteFilesize = filesize;
                             this->enterPassiveMode();
                         });
    }

    void DownloadFileTask::start()
    {
        isReset = false;
        isSetStop = false;
        session.connectAndLogin();
    }

    void DownloadFileTask::resume()
    {
        isReset = true;
        isSetStop = false;
        std::ifstream ifs(localFilepath, std::ios_base::binary);
        this->downloadOffset = utils::getFilesize(ifs);
        ofs.seekp(downloadOffset);
        session.connectAndLogin();
    }

    void DownloadFileTask::stop()
    {
        isSetStop = true;
        utils::asyncAwait([this]() {
            std::string sendCmd = "ABOR\r\n";
            send(session.getControlSock(), sendCmd.c_str(), sendCmd.length(),
                 0);
            std::string recvMsg;
            //两种情况：1. 服务器返回 226/225；2. 服务器先返回 426，再返回
            // 226/225 为了省事这里不检查返回码，只是把缓冲区中的消息吃掉
            recvMultipleMsg(session.getControlSock(), std::regex("."), recvMsg);
        });
        //关闭数据连接和控制连接
        this->quit();
    }

    void DownloadFileTask::enterPassiveMode()
    {
        std::string errorMsg;
        std::string dataHostname;
        int port;
        //先尝试 PASV 模式
        auto pasvRet = utils::asyncAwait<CmdToServerRet>(
            putServerIntoPasvMode, session.getControlSock(), port, dataHostname,
            errorMsg);
        if (pasvRet == CmdToServerRet::SUCCEEDED)
            this->dataSocketConnect(dataHostname, port);
        else if (pasvRet == CmdToServerRet::FAILED_WITH_MSG)
        {
            //返回码为500，必须要用EPSV模式
            if (std::regex_search(errorMsg, std::regex(R"(^500.*)")))
            {
                auto epsvRet = utils::asyncAwait<CmdToServerRet>(
                    putServerIntoEpsvMode, session.getControlSock(), port,
                    errorMsg);
                if (epsvRet == CmdToServerRet::SUCCEEDED)
                    this->dataSocketConnect(session.getHostname(), port);
                else if (epsvRet == CmdToServerRet::FAILED_WITH_MSG)
                    emit downloadFailedWithMsg(errorMsg);
                else
                    emit downloadFailed();
            }
            else
                emit downloadFailedWithMsg(errorMsg);
        }
        else
            emit downloadFailed();
    }

    void DownloadFileTask::quit()
    {
        if (dataSocket != INVALID_SOCKET)
        {
            closesocket(dataSocket);
            dataSocket = INVALID_SOCKET;
        }
        isDataConnected = false;
        session.quit();
    }

    void DownloadFileTask::dataSocketConnect(const std::string &hostname,
                                             int port)
    {
        if (isSetStop)
            return;

        auto res = utils::asyncAwait<ConnectToServerRes>(
            connectToServer, dataSocket, hostname, std::to_string(port),
            DownloadFileTask::SENDTIMEOUT, DownloadFileTask::RECVTIMEOUT);
        if (res != ConnectToServerRes::SUCCEEDED)
            emit downloadFailed();
        else
        {
            isDataConnected = true;
            this->downloadRequest();
        }
    }

    void DownloadFileTask::downloadRequest()
    {
        if (isSetStop)
            return;
        std::string errorMsg;
        if (isReset)
        {
            auto resetRes = utils::asyncAwait<CmdToServerRet>(
                requestRestFromServer, session.getControlSock(), downloadOffset,
                errorMsg);
            if (!isSetStop && resetRes != CmdToServerRet::SUCCEEDED)
            {
                if (resetRes == CmdToServerRet::FAILED_WITH_MSG)
                    emit downloadFailedWithMsg(std::move(errorMsg));
                else
                    emit downloadFailed();
                return;
            }
        }

        CmdToServerRet retrRes = utils::asyncAwait<CmdToServerRet>(
            requestRetrFromFromServer, session.getControlSock(), remoteFilepath,
            errorMsg);
        if (!isSetStop)
        {
            if (retrRes == CmdToServerRet::SUCCEEDED)
            {
                emit downloadStarted();
                this->downloadFileData(); //开始传输文件内容
            }
            else if (retrRes == CmdToServerRet::FAILED_WITH_MSG)
                emit downloadFailedWithMsg(std::move(errorMsg));
            else // res == SEND_FAILED || res == RECV_FAILED
                emit downloadFailed();
        }
    }

    void DownloadFileTask::downloadFileData()
    {
        int percent = 0, lastPercent = 0;
        QFuture<DownloadFileDataRes> downFuture =
            QtConcurrent::run([this, &percent]() {
                return downloadFileDataFromServer(dataSocket, ofs,
                                                  remoteFilesize, percent);
            });
        while (!downFuture.isFinished())
        {
            QApplication::processEvents();
            if (percent != lastPercent)
            {
                emit percentSync(percent);
                lastPercent = percent;
            }
        }

        //关闭数据连接
        closesocket(dataSocket);
        dataSocket = INVALID_SOCKET;
        isDataConnected = false;

        if (!isSetStop)
        {
            auto downRes = downFuture.result();
            if (downRes == DownloadFileDataRes::SUCCEEDED)
                emit downloadSucceed();
            else if (downRes == DownloadFileDataRes::READ_FILE_ERROR)
                emit readFileError();
            else
                emit downloadFailed();
        }

        //关闭控制连接
        session.quit();
    }

} // namespace ftpclient
