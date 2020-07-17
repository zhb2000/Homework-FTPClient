#include "../include/ListTask.h"
#include "../include/MyUtils.h"
#include <QtDebug>

namespace ftpclient
{

    ListTask::Res
    ListTask::getListStrings(std::vector<std::string> &listStrings,
                             std::string &errorMsg)
    {
        Res result = this->start(errorMsg);
        if (result == Res::SUCCEEDED)
            listStrings = utils::splitLines(recvListString);
        return result;
    }

    ListTask::Res ListTask::start(std::string &errorMsg)
    {
        return this->enterPassiveMode(errorMsg);
    }

    ListTask::Res ListTask::enterPassiveMode(std::string &errorMsg)
    {
        std::string ipv4Hostname;
        int port;
        std::string recvErrorMsg;
        auto pasvRet = putServerIntoPasvMode(session.getControlSock(), port,
                                             ipv4Hostname, recvErrorMsg);
        if (pasvRet == CmdToServerRet::SUCCEEDED)
            return this->dataConnect(ipv4Hostname, port, errorMsg);
        else if (pasvRet == CmdToServerRet::FAILED_WITH_MSG)
        {
            //返回码为500，必须要用EPSV模式
            if (std::regex_search(recvErrorMsg, std::regex(R"(^500.*)")))
            {
                auto epsvRet = putServerIntoEpsvMode(session.getControlSock(),
                                                     port, recvErrorMsg);
                if (epsvRet == CmdToServerRet::SUCCEEDED)
                    return this->dataConnect(session.getHostname(), port,
                                             errorMsg);
                else if (epsvRet == CmdToServerRet::FAILED_WITH_MSG)
                {
                    errorMsg = std::move(recvErrorMsg);
                    return Res::FAILED_WITH_MSG;
                }
                else
                    return Res::FAILED;
            }
            else
            {
                errorMsg = std::move(recvErrorMsg);
                return Res::FAILED_WITH_MSG;
            }
        }
        else
            return Res::FAILED;
    }

    ListTask::Res ListTask::dataConnect(const std::string &hostname, int port,
                                        std::string &errorMsg)
    {
        auto res = connectToServer(dataSock, hostname, std::to_string(port),
                                   ListTask::SOCKET_SEND_TIMEOUT,
                                   ListTask::SOCKET_RECV_TIMEOUT);
        //数据连接建立失败
        if (res != ConnectToServerRes::SUCCEEDED)
            return Res::FAILED;
        //数据连接建立成功，向服务器发 LIST 命令
        else
        {
            this->isConnect = true;
            return this->requestForList(errorMsg);
        }
    }

    ListTask::Res ListTask::requestForList(std::string &errorMsg)
    {
        std::string recvErrorMsg;
        auto res =
            requestToListOnServer(session.getControlSock(), dir, recvErrorMsg);
        if (res == CmdToServerRet::SUCCEEDED)
            return this->getListRawData(errorMsg);
        else if (res == CmdToServerRet::FAILED_WITH_MSG)
        {
            errorMsg = std::move(recvErrorMsg);
            return Res::FAILED_WITH_MSG;
        }
        else
            return Res::FAILED_WITH_MSG;
    }

    ListTask::Res ListTask::getListRawData(std::string &errorMsg)
    {
        auto closeDataSock = [&]() {
            closesocket(dataSock);
            dataSock = INVALID_SOCKET;
            isConnect = false;
        };
        recvListString.clear();
        int recvLen = utils::recvUntilClose(dataSock, recvListString);
        // TODO(zhb) 接收目录时的recvLen
        if (recvLen >= 0)
        {
            closeDataSock();
            return this->recvMsgAfterTransfer(errorMsg);
        }
        else
        {
            closeDataSock();
            return Res::FAILED;
        }
    }

    ListTask::Res ListTask::recvMsgAfterTransfer(std::string &errorMsg)
    {
        // 目录传输结束，用控制连接接收服务器消息
        std::string recvMsg;
        int recvLen = utils::recvFtpMsg(session.getControlSock(), recvMsg);
        if (recvLen <= 0)
            return Res::FAILED;
        //正常为 226 Successfully transferred "dir"
        if (!std::regex_search(recvMsg, std::regex(R"(^226.*)")))
        {
            errorMsg = std::move(recvMsg);
            return Res::FAILED_WITH_MSG;
        }
        return Res::SUCCEEDED;
    }

} // namespace ftpclient
