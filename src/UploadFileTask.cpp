#include "../include/UploadFileTask.h"
#include "../include/FTPFunction.h"
#include "../include/MyUtils.h"
#include <QApplication>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <memory>
#include <regex>

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
        int iResult = utils::recv_all(controlSock, recvMsg);
        if (iResult <= 0)
            return RecvMsgAfterUpRes::FAILED;
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
          isConnected(false)
    {
    }

    UploadFileTask::~UploadFileTask()
    {
        if (!isConnected)
            closesocket(dataSock);
    }

    void UploadFileTask::start() { this->enterPassiveMode(); }

    void UploadFileTask::stop()
    {
        closesocket(dataSock);
        this->isConnected = false;
    }

    void UploadFileTask::enterPassiveMode()
    {
        std::string dataHostname;
        int port;
        std::string errorMsg;

        //先尝试 PASV 模式
        QFuture<CmdToServerRet> pasvFuture = QtConcurrent::run([&]() {
            return putServerIntoPasvMode(session.getControlSock(), port,
                                         dataHostname, errorMsg);
        });
        while (!pasvFuture.isFinished())
            QApplication::processEvents();

        auto pasvRet = pasvFuture.result();
        if (pasvRet == CmdToServerRet::SUCCEEDED)
            this->dataConnect(dataHostname, port);
        else if (pasvRet == CmdToServerRet::FAILED_WITH_MSG)
        {
            //返回码为500，必须要用EPSV模式
            if (std::regex_search(errorMsg, std::regex(R"(^500.*)")))
            {
                QFuture<CmdToServerRet> epsvFuture = QtConcurrent::run([&]() {
                    return putServerIntoEpsvMode(session.getControlSock(), port,
                                                 errorMsg);
                });
                while (!epsvFuture.isFinished())
                    QApplication::processEvents();

                auto epsvRes = epsvFuture.result();
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
        //尝试与服务器建立数据连接
        QFuture<ConnectToServerRes> future = QtConcurrent::run([&]() {
            return connectToServer(dataSock, hostname, std::to_string(port),
                                   UploadFileTask::SOCKET_SEND_TIMEOUT,
                                   UploadFileTask::SOCKET_RECV_TIMEOUT);
        });
        while (!future.isFinished())
            QApplication::processEvents();

        //数据连接建立失败，发射 uploadFailed 信号
        if (future.result() != ConnectToServerRes::SUCCEEDED)
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
        std::string errorMsg;
        QFuture<RequestToUpRes> future = QtConcurrent::run([&]() {
            return requestToUploadToServer(session.getControlSock(),
                                           remoteFileName, errorMsg);
        });
        while (!future.isFinished())
            QApplication::processEvents();

        // TODO(zhb) 先检查同名文件是否存在于服务器上

        auto res = future.result();
        if (res == RequestToUpRes::SUCCEEDED)
        {
            //服务器同意上传文件
            emit uploadStarted();   //发射 uploadStarted 信号
            this->uploadFileData(); //开始传输文件内容
        }
        else if (res == RequestToUpRes::FAILED_WITH_MSG)
            emit uploadFailedWithMsg(std::move(errorMsg));
        else // res == SEND_FAILED || res == RECV_FAILED
            emit uploadFailed();
    }

    void UploadFileTask::uploadFileData()
    {
        std::string errorMsg;
        QFuture<UploadFileDataRes> upFuture = QtConcurrent::run(
            [&]() { return uploadFileDataToServer(dataSock, ifs); });
        while (!upFuture.isFinished())
        {
            QApplication::processEvents();
            // TODO(zhb) 上传百分比
        }

        //关闭数据连接
        closesocket(this->dataSock);
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
        else if (upRes == UploadFileDataRes::SEND_FAILED)
            emit uploadFailed();
        else // upRes == READ_FILE_ERROR
            emit readFileError();
    }

} // namespace ftpclient
