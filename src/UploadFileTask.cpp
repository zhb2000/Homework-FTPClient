#include "../include/UploadFileTask.h"
#include "../include/FTPFunction.h"
#include "../include/MyUtils.h"
#include "../include/RunAsyncAwait.h"
#include <QApplication>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <memory>
#include <regex>

const int ftpclient::UploadFileTask::SOCKET_SEND_TIMEOUT;
const int ftpclient::UploadFileTask::SOCKET_RECV_TIMEOUT;

namespace
{
    enum class RecvMsgAfterUpRes
    {
        SUCCEEDED,
        FAILED_WITH_MSG,
        FAILED
    };

    /**
     * @brief 上传结束后接收服务器的消息
     * @author zhb
     * @param controlSock 控制连接
     * @param errorMsg 出口参数，来自服务的错误消息
     * @return 结果状态码
     */
    RecvMsgAfterUpRes recvMsgAfterUpload(SOCKET controlSock,
                                         std::string errorMsg)
    {
        std::string recvMsg;
        int iResult = utils::recvAll(controlSock, recvMsg);
        if (iResult <= 0)
            return RecvMsgAfterUpRes::FAILED;
        // 226 Successfully transferred "filename"
        //检查返回码是否为226
        if (!std::regex_search(recvMsg, std::regex(R"(^226.*)")))
        {
            errorMsg = std::move(recvMsg);
            return RecvMsgAfterUpRes::FAILED_WITH_MSG;
        }
        return RecvMsgAfterUpRes::SUCCEEDED;
    }
} // namespace

namespace ftpclient
{

    UploadFileTask::UploadFileTask(FTPSession &session, std::string fileName,
                                   std::ifstream &ifs)
        : session(session),
          remoteFileName(std::move(fileName)),
          ifs(ifs),
          dataSock(INVALID_SOCKET),
          isConnected(false),
          isStop(false)
    {
    }

    UploadFileTask::~UploadFileTask()
    {
        if (dataSock != INVALID_SOCKET)
        {
            closesocket(dataSock);
            dataSock = INVALID_SOCKET;
        }
        isConnected = false;
    }

    void UploadFileTask::start() { this->enterPassiveMode(); }

    void UploadFileTask::stop()
    {
        isStop = true;
        if (dataSock != INVALID_SOCKET)
        {
            closesocket(dataSock);
            dataSock = INVALID_SOCKET;
        }
        isConnected = false;
    }

    void UploadFileTask::enterPassiveMode()
    {
        std::string dataHostname;
        int port;
        std::string errorMsg;

        //先尝试 PASV 模式
        auto pasvRet = utils::asyncAwait<CmdToServerRet>(
            putServerIntoPasvMode, session.getControlSock(), port, dataHostname,
            errorMsg);
        if (pasvRet == CmdToServerRet::SUCCEEDED)
            this->dataConnect(dataHostname, port);
        else if (pasvRet == CmdToServerRet::FAILED_WITH_MSG)
        {
            //返回码为500，必须要用EPSV模式
            if (std::regex_search(errorMsg, std::regex(R"(^500.*)")))
            {
                auto epsvRes = utils::asyncAwait<CmdToServerRet>(
                    putServerIntoEpsvMode, session.getControlSock(), port,
                    errorMsg);
                // EPSV模式下，数据连接的主机名与控制连接的相同
                if (epsvRes == CmdToServerRet::SUCCEEDED)
                    this->dataConnect(session.getHostName(), port);
                else if (epsvRes == CmdToServerRet::FAILED_WITH_MSG)
                    emit uploadFailedWithMsg(std::move(errorMsg));
                else // SEND_FAILED or RECV_FAILED
                    emit uploadFailed();
            }
            else
                emit uploadFailedWithMsg(std::move(errorMsg));
        }
        else // SEND_FAILED or RECV_FAILED
            emit uploadFailed();
    }

    void UploadFileTask::dataConnect(const std::string &hostname, int port)
    {
        if (isStop)
            return;

        auto res = utils::asyncAwait<ConnectToServerRes>(
            connectToServer, dataSock, hostname, std::to_string(port),
            UploadFileTask::SOCKET_SEND_TIMEOUT,
            UploadFileTask::SOCKET_RECV_TIMEOUT);
        //数据连接建立失败，发射 uploadFailed 信号
        if (res != ConnectToServerRes::SUCCEEDED)
            emit uploadFailed();
        //数据连接建立成功，向服务器发 STOR 命令请求上传
        else
        {
            this->isConnected = true;
            this->uploadRequest();
        }
    }

    void UploadFileTask::uploadRequest()
    {
        if (isStop)
            return;
        std::string errorMsg;
        auto res = utils::asyncAwait<CmdToServerRet>(requestToUploadToServer,
                                                     session.getControlSock(),
                                                     remoteFileName, errorMsg);
        if (res == CmdToServerRet::SUCCEEDED)
        {
            //服务器同意上传文件
            emit uploadStarted();   //发射 uploadStarted 信号
            this->uploadFileData(); //开始传输文件内容
        }
        else if (!isStop)
        {
            if (res == CmdToServerRet::FAILED_WITH_MSG)
                emit uploadFailedWithMsg(std::move(errorMsg));
            else // res == SEND_FAILED || res == RECV_FAILED
                emit uploadFailed();
        }
    }

    void UploadFileTask::uploadFileData()
    {
        std::string errorMsg;
        int percent = 0, lastPercent = 0;
        QFuture<UploadFileDataRes> upFuture = QtConcurrent::run(
            [&]() { return uploadFileDataToServer(dataSock, ifs, percent); });
        while (!upFuture.isFinished())
        {
            QApplication::processEvents();
            if (percent != lastPercent)
            {
                emit uploadPercentage(percent);
                lastPercent = percent;
            }
        }

        //关闭数据连接
        closesocket(this->dataSock);
        this->dataSock = INVALID_SOCKET;
        this->isConnected = false;

        auto upRes = upFuture.result();
        if (upRes == UploadFileDataRes::SUCCEEDED)
        {
            // 上传结束，用控制端口接收服务器消息
            QFuture<RecvMsgAfterUpRes> recvFuture = QtConcurrent::run([&]() {
                return recvMsgAfterUpload(session.getControlSock(), errorMsg);
            });
            while (!recvFuture.isFinished())
                QApplication::processEvents();

            auto recvRes = recvFuture.result();
            if (recvRes == RecvMsgAfterUpRes::SUCCEEDED)
                emit uploadSucceeded();
            else if (recvRes == RecvMsgAfterUpRes::FAILED_WITH_MSG)
                emit uploadFailedWithMsg(std::move(errorMsg));
            else // recvRes==FAILED
                emit uploadFailed();
        }
        else if (!isStop)
        {
            if (upRes == UploadFileDataRes::SEND_FAILED)
                emit uploadFailed();
            else // upRes == READ_FILE_ERROR
                emit readFileError();
        }
    }

} // namespace ftpclient
