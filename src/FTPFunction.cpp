#include "../include/FTPFunction.h"
#include "../include/MyUtils.h"
#include "../include/ScopeGuard.h"
#include <QtDebug>
#include <cstdio>
#include <cstring>
#include <memory>
#include <regex>
#include <ws2tcpip.h>

using std::unique_ptr;
using utils::ScopeGuard;

namespace
{
    /**
     * @brief 设置阻塞式send()的超时时间
     * @author zhb
     * @param sock 被设置的socket
     * @param timeout 超时时间(ms)
     * @return 设置是否成功
     */
    bool setSendTimeout(SOCKET sock, int timeout)
    {
        int iResult;
        iResult = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO,
                             (const char *)&timeout, sizeof(timeout));
        if (iResult != 0)
            return false;
        else
            return true;
    }

    /**
     * @brief 设置阻塞式recv()的超时时间
     * @author zhb
     * @param sock 被设置的socket
     * @param timeout 超时时间(ms)
     * @return 设置是否成功
     */
    bool setRecvTimeout(SOCKET sock, int timeout)
    {
        int iResult;
        iResult = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
                             (const char *)&timeout, sizeof(timeout));
        if (iResult != 0)
            return false;
        else
            return true;
    }
} // namespace

namespace ftpclient
{

    ConnectToServerRes connectToServer(SOCKET &sock,
                                       const std::string &hostname,
                                       const std::string &port, int sendTimeout,
                                       int recvTimeout)
    {
        WSADATA wsaData;
        int iResult;
        //初始化Winsock
        iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0)
            return ConnectToServerRes::WSAStartup_FAILED;
        // WSACleanup()用于结束对WS2_32 DLL的使用
        ScopeGuard guardCleanWSA([]() { WSACleanup(); });

        addrinfo *result = nullptr;
        addrinfo hints;
        ZeroMemory(&hints, sizeof(hints)); //结构体初始化
        hints.ai_family = AF_UNSPEC;       //未指定，IPv4和IPv6均可
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        //设定服务器地址和端口号
        iResult = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &result);
        if (iResult != 0)
            return ConnectToServerRes::getaddrinfo_FAILED;
        ScopeGuard guardFreeAddrinfo([=]() { freeaddrinfo(result); });

        sock = INVALID_SOCKET;
        for (addrinfo *ptr = result; ptr != nullptr; ptr = ptr->ai_next)
        {
            //创建用于连接服务器的socket
            sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
            if (sock == INVALID_SOCKET)
                return ConnectToServerRes::socket_FAILED;

            //尝试连接服务器
            iResult = connect(sock, ptr->ai_addr, (int)ptr->ai_addrlen);
            if (iResult == SOCKET_ERROR)
            {
                closesocket(sock);
                sock = INVALID_SOCKET;
                continue;
            }
            else
                break; //连接服务器成功
        }
        if (sock == INVALID_SOCKET)
            return ConnectToServerRes::UNABLE_TO_CONNECT_TO_SERVER;
        else
        {
            if (sendTimeout >= 0)
                setSendTimeout(sock, sendTimeout);
            if (recvTimeout >= 0)
                setRecvTimeout(sock, recvTimeout);
            guardCleanWSA.dismiss();
            return ConnectToServerRes::SUCCEEDED;
        }
    }

    RecvMultRes recvMultipleMsg(SOCKET controlSock,
                                const std::regex &matchRegex, std::string &msg)
    {
        msg.clear();
        std::string recvMsg; //一条FTP消息
        bool hasMsg = false;
        int iResult;
        while (true)
        {
            recvMsg.clear();
            iResult = utils::recvFtpMsg(controlSock, recvMsg);
            if (iResult > 0)
            {
                //检查返回码
                if (!std::regex_search(recvMsg, matchRegex))
                {
                    msg = std::move(recvMsg);
                    return RecvMultRes::FAILED_WITH_MSG;
                }
                else
                {
                    hasMsg = true;
                    msg += recvMsg;
                }
            }
            else
            {
                int errorCode = WSAGetLastError();
                if (iResult < 0 && errorCode != WSAETIMEDOUT)
                    return RecvMultRes::FAILED;
                else
                    break;
            }
        }
        return hasMsg ? RecvMultRes::SUCCEEDED : RecvMultRes::FAILED;
    }

    RecvMultRes recvWelcomMsg(SOCKET controlSock, std::string &msg)
    {
        return recvMultipleMsg(controlSock, std::regex(R"(^220[-\s]+)"), msg);
    }

    RecvMultRes recvLoginSucceededMsg(SOCKET controlSock, std::string &msg)
    {
        return recvMultipleMsg(controlSock, std::regex(R"(^230[-\s])"), msg);
    }

    CmdToServerRet cmdToServer(SOCKET controlSock, const std::string &sendCmd,
                               const std::regex &matchRegex,
                               std::string &recvMsg)
    {
        int iResult;
        //向服务器发送命令
        iResult = send(controlSock, sendCmd.c_str(), sendCmd.length(), 0);
        if (iResult == SOCKET_ERROR)
            return CmdToServerRet::SEND_FAILED;
        //接收服务器响应码和信息
        iResult = utils::recvFtpMsg(controlSock, recvMsg);
        if (iResult <= 0)
            return CmdToServerRet::RECV_FAILED;
        //检查响应码和信息是否符合要求
        if (!std::regex_search(recvMsg, matchRegex))
            return CmdToServerRet::FAILED_WITH_MSG;
        return CmdToServerRet::SUCCEEDED;
    }

    CmdToServerRet loginToServer(SOCKET controlSock,
                                 const std::string &username,
                                 const std::string &password,
                                 std::string &errorMsg)
    {
        //命令 "USER username\r\n"
        std::string userCmd = "USER " + username + "\r\n";
        std::string recvMsg;
        //正常为"331 User name okay, need password."
        //返回码是否为331
        std::regex userRegex(R"(331\s.*)");
        auto ret = cmdToServer(controlSock, userCmd, userRegex, recvMsg);
        if (ret == CmdToServerRet::SUCCEEDED)
        {
            // 命令 "PASS password\r\n"
            std::string passCmd = "PASS " + password + "\r\n";
            //向服务器发送命令
            int iResult =
                send(controlSock, passCmd.c_str(), passCmd.length(), 0);
            if (iResult == SOCKET_ERROR)
                return CmdToServerRet::SEND_FAILED;
            //返回的消息可能有多条
            auto passRes = recvLoginSucceededMsg(controlSock, recvMsg);
            if (passRes == RecvMultRes::SUCCEEDED)
                return CmdToServerRet::SUCCEEDED;
            else if (passRes == RecvMultRes::FAILED_WITH_MSG)
            {
                errorMsg = std::move(recvMsg);
                return CmdToServerRet::FAILED_WITH_MSG;
            }
            else
                return CmdToServerRet::RECV_FAILED;
        }
        else if (ret == CmdToServerRet::FAILED_WITH_MSG)
        {
            errorMsg = std::move(recvMsg);
            return ret;
        }
        else
            return ret;
    }

    CmdToServerRet putServerIntoPasvMode(SOCKET controlSock, int &port,
                                         std::string &hostname,
                                         std::string &errorMsg)
    {
        //命令"PASV\r\n"
        std::string sendCmd = "PASV\r\n";
        std::string recvMsg;
        //正常为"227 Entering passive mode (h1,h2,h3,h4,p1,p2)"
        //检查返回码是否为227
        std::regex e(R"(^227\s.*\(\d+,\d+,\d+,\d+,\d+,\d+\))");
        auto ret = cmdToServer(controlSock, sendCmd, e, recvMsg);
        if (ret == CmdToServerRet::SUCCEEDED)
            std::tie(hostname, port) = utils::getIPAndPortForPSAV(recvMsg);
        else if (ret == CmdToServerRet::FAILED_WITH_MSG)
            errorMsg = std::move(recvMsg);
        return ret;
    }

    CmdToServerRet putServerIntoEpsvMode(SOCKET controlSock, int &port,
                                         std::string &errorMsg)
    {
        //命令"EPSV\r\n"
        std::string sendCmd = "EPSV\r\n";
        std::string recvMsg;
        //正常为"229 Entering Extended Passive Mode (|||port|)"
        //检查返回码是否为229
        std::regex e(R"(^229\s.*\(\|\|\|\d+\|\))");
        auto ret = cmdToServer(controlSock, sendCmd, e, recvMsg);
        if (ret == CmdToServerRet::SUCCEEDED)
            port = utils::getPortForEPSV(recvMsg);
        else if (ret == CmdToServerRet::FAILED_WITH_MSG)
            errorMsg = std::move(recvMsg);
        return ret;
    }

    CmdToServerRet requestToUploadToServer(SOCKET controlSock, bool isAppend,
                                           const std::string &remoteFilepath,
                                           std::string &errorMsg)
    {
        //命令"STOR filename\r\n" 或 "APPE filename\r\n"
        std::string sendCmd =
            (isAppend ? "APPE " : "STOR ") + remoteFilepath + "\r\n";
        std::string recvMsg;
        //正常为"150 Opening data connection."
        //检查返回码是否为150或125
        std::regex e(R"(^(150|125)\s+)");
        auto ret = cmdToServer(controlSock, sendCmd, e, recvMsg);
        if (ret == CmdToServerRet::FAILED_WITH_MSG)
            errorMsg = std::move(recvMsg);
        return ret;
    }

    CmdToServerRet requestRetrFromFromServer(SOCKET controlSock,
                                             const std::string &remoteFilepath,
                                             std::string &errorMsg)
    {
        std::string sendCmd = "RETR " + remoteFilepath + "\r\n";
        std::string recvMsg;
        //正常为"150 Opening data connection."
        //检查返回码是否为150或125
        std::regex e(R"(^(150|125)\s+)");
        auto ret = cmdToServer(controlSock, sendCmd, e, recvMsg);
        if (ret == CmdToServerRet::FAILED_WITH_MSG)
            errorMsg = std::move(recvMsg);
        return ret;
    }

    CmdToServerRet requestRestFromServer(SOCKET controlSock, long long offset,
                                         std::string &errorMsg)
    {
        std::string sendCmd = "REST " + std::to_string(offset) + "\r\n";
        std::string recvMsg;
        //检查返回码是否为350
        std::regex e(R"(^350\s+)");
        auto ret = cmdToServer(controlSock, sendCmd, e, recvMsg);
        if (ret == CmdToServerRet::FAILED_WITH_MSG)
            errorMsg = std::move(recvMsg);
        return ret;
    }

    CmdToServerRet getFilesizeOnServer(SOCKET controlSock,
                                       const std::string &filename,
                                       long long &filesize,
                                       std::string &errorMsg)
    {
        //命令"SIZE filename\r\n"
        std::string sendCmd = "SIZE " + filename + "\r\n";
        //检查返回码是否为"213 size"
        std::regex e(R"(^213\s+\d+)");
        std::string recvMsg;
        auto ret = cmdToServer(controlSock, sendCmd, e, recvMsg);
        if (ret == CmdToServerRet::SUCCEEDED)
            filesize = utils::getSizeFromMsg(recvMsg);
        else if (ret == CmdToServerRet::FAILED_WITH_MSG)
            errorMsg = std::move(recvMsg);
        return ret;
    }

    CmdToServerRet getWorkingDirectory(SOCKET controlSock, std::string &dir,
                                       std::string &errorMsg)
    {
        //命令"PWD\r\n"
        std::string sendCmd = "PWD\r\n";
        std::string recvMsg;
        //正常为 257 "dir" is current directory.
        //检查返回码是否为257
        std::regex e(R"(^257\s+".+")");
        auto ret = cmdToServer(controlSock, sendCmd, e, recvMsg);
        if (ret == CmdToServerRet::SUCCEEDED)
            dir = utils::getDirFromMsg(recvMsg);
        else if (ret == CmdToServerRet::FAILED_WITH_MSG)
            errorMsg = std::move(recvMsg);
        return ret;
    }

    CmdToServerRet changeWorkingDirectory(SOCKET controlSock,
                                          const std::string &dir,
                                          std::string &errorMsg)
    {
        //命令"CWD dir\r\n"
        std::string sendCmd = "CWD " + dir + "\r\n";
        std::string recvMsg;
        //正常为"250 CWD successful"
        //检查返回码是否为250
        std::regex e(R"(^250\s+)");
        auto ret = cmdToServer(controlSock, sendCmd, e, recvMsg);
        if (ret == CmdToServerRet::FAILED_WITH_MSG)
            errorMsg = std::move(recvMsg);
        return ret;
    }

    CmdToServerRet setBinaryOrAsciiTransferMode(SOCKET controlSock,
                                                bool binaryMode,
                                                std::string &errorMsg)
    {
        //命令"TYPE I\r\n"或"TYPE A\r\n";
        std::string param = binaryMode ? "I" : "A";
        std::string sendCmd = "TYPE " + param + "\r\n";
        std::string recvMsg;
        //正常为"200 Type set to mode"
        //检查返回码是否为200
        std::regex e(R"(^200\s+)");
        auto ret = cmdToServer(controlSock, sendCmd, e, recvMsg);
        if (ret == CmdToServerRet::FAILED_WITH_MSG)
            errorMsg = std::move(recvMsg);
        return ret;
    }

    CmdToServerRet sendNoopToServer(SOCKET controlSock, std::string &errorMsg)
    {
        //命令"NOOP\r\n"
        std::string sendCmd = "NOOP\r\n";
        std::string recvMsg;
        //正常为"200 OK"
        //检查返回码是否为200
        std::regex e(R"(^200\s+)");
        auto ret = cmdToServer(controlSock, sendCmd, e, recvMsg);
        if (ret == CmdToServerRet::FAILED_WITH_MSG)
            errorMsg = std::move(recvMsg);
        return ret;
    }

    CmdToServerRet deleteFileOnServer(SOCKET controlSock,
                                      const std::string &filename,
                                      std::string &errorMsg)
    {
        //命令"DELE filename\r\n"
        std::string sendCmd = "DELE " + filename + "\r\n";
        std::string recvMsg;
        //正常为"250 File deleted successfully"
        //检查返回码是否为250
        std::regex e(R"(^250\s+)");
        auto ret = cmdToServer(controlSock, sendCmd, e, recvMsg);
        if (ret == CmdToServerRet::FAILED_WITH_MSG)
            errorMsg = std::move(recvMsg);
        return ret;
    }

    CmdToServerRet makeDirectoryOnServer(SOCKET controlSock,
                                         const std::string &dir,
                                         std::string &errorMsg)
    {
        //命令"MKD dir\r\n"
        std::string sendCmd = "MKD " + dir + "\r\n";
        std::string recvMsg;
        //正常为 257 "dir" created successfully
        //检查返回码是否为257
        std::regex e(R"(^257\s+)");
        auto ret = cmdToServer(controlSock, sendCmd, e, recvMsg);
        if (ret == CmdToServerRet::FAILED_WITH_MSG)
            errorMsg = std::move(recvMsg);
        return ret;
    }

    CmdToServerRet removeDirectoryOnServer(SOCKET controlSock,
                                           const std::string &dir,
                                           std::string &errorMsg)
    {
        //命令"RMD dir\r\n"
        std::string sendCmd = "RMD " + dir + "\r\n";
        std::string recvMsg;
        //正常为"250 Directory deleted successfully"
        //检查返回码是否为250
        std::regex e(R"(^250\s+)");
        auto ret = cmdToServer(controlSock, sendCmd, e, recvMsg);
        if (ret == CmdToServerRet::FAILED_WITH_MSG)
            errorMsg = std::move(recvMsg);
        return ret;
    }

    CmdToServerRet renameFileOnServer(SOCKET controlSock,
                                      const std::string &oldName,
                                      const std::string &newName,
                                      std::string &errorMsg)
    {
        std::string recvMsg;
        //命令"RNFR filename\r\n"
        std::string rnfrCmd = "RNFR " + oldName + "\r\n";
        //正常为"350 File exists, ready for destination name."
        std::regex rnfrE(R"(^350\s+)");
        auto ret = cmdToServer(controlSock, rnfrCmd, rnfrE, recvMsg);
        if (ret == CmdToServerRet::SUCCEEDED)
        {
            //命令"RNTO filename\r\n"
            std::string rntoCmd = "RNTO " + newName + "\r\n";
            //正常为"250 file renamed successfully"
            std::regex rntoE(R"(^250\s+)");
            ret = cmdToServer(controlSock, rntoCmd, rntoE, errorMsg);
            if (ret == CmdToServerRet::FAILED_WITH_MSG)
                errorMsg = std::move(recvMsg);
            return ret;
        }
        else if (ret == CmdToServerRet::FAILED_WITH_MSG)
        {
            errorMsg = std::move(recvMsg);
            return ret;
        }
        else
            return ret;
    }

    CmdToServerRet requestToListOnServer(SOCKET controlSock,
                                         const std::string &dir,
                                         bool isNameList, std::string &errorMsg)
    {
        //命令"LIST dir\r\n"
        std::string sendCmd = (isNameList ? "NLST " : "LIST ") + dir + "\r\n";
        std::string recvMsg;
        //正常为 150 Opening data channel for directory listing of "dir"
        //检查返回码是否为150或125
        std::regex e(R"(^(150|125)\s+)");
        auto ret = cmdToServer(controlSock, sendCmd, e, recvMsg);
        if (ret == CmdToServerRet::FAILED_WITH_MSG)
            errorMsg = std::move(recvMsg);
        return ret;
    }

    UploadFileDataRes uploadFileDataToServer(SOCKET dataSock,
                                             std::ifstream &ifs, int &percent)
    {
        if (!ifs.is_open())
            return UploadFileDataRes::READ_FILE_ERROR;
        long long filesize = utils::getFilesize(ifs);
        long long totalSend = ifs.tellg(); //已发送的字节总数
        const int sendBufLen = 1024;
        unique_ptr<char[]> sendBuffer(new char[sendBufLen]);
        int iResult;
        //开始上传文件
        while (!ifs.eof())
        {
            ifs.read(sendBuffer.get(), sendBufLen); //客户端读文件，读取一块
            int readLen = int(ifs.gcount());        //刚刚读取的字节数
            iResult = send(dataSock, sendBuffer.get(), readLen, 0);
            if (iResult == SOCKET_ERROR)
                return UploadFileDataRes::SEND_FAILED;
            else
            {
                totalSend += readLen;
                percent = int(totalSend * 100 / filesize);
            }
        }

        return UploadFileDataRes::SUCCEEDED;
    }

    DownloadFileDataRes downloadFileDataFromServer(SOCKET dataSock,
                                                   std::ofstream &ofs,
                                                   long long remoteFilesize,
                                                   int &percent)
    {
        if (!ofs.is_open())
            return DownloadFileDataRes::READ_FILE_ERROR;
        long long totalRecv = ofs.tellp(); //已接收的字节总数
        const int recvBufLen = 1024;
        unique_ptr<char[]> recvBuffer(new char[recvBufLen]);
        int iResult;
        while (true)
        {
            memset(recvBuffer.get(), 0, recvBufLen);
            iResult = recv(dataSock, recvBuffer.get(), recvBufLen, 0);
            if (iResult > 0)
            {
                totalRecv += iResult;
                ofs.write(recvBuffer.get(), iResult);
                percent = int(totalRecv * 100 / remoteFilesize);
            }
            else if (iResult == 0)
                break;
            else
                return DownloadFileDataRes::RECV_FAILED;
        }
        return DownloadFileDataRes::SUCCEEDED;
    }

} // namespace ftpclient
