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
                                       const std::string &hostName,
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
        iResult = getaddrinfo(hostName.c_str(), port.c_str(), &hints, &result);
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

    LoginToServerRes loginToServer(SOCKET controlSock,
                                   const std::string &userName,
                                   const std::string &password,
                                   std::string &errorMsg)
    {
        const int sendBufLen = 1024;
        unique_ptr<char[]> sendBuffer(new char[sendBufLen]);
        std::string recvMsg;

        int iResult;
        //命令 "USER username\r\n"
        sprintf(sendBuffer.get(), "USER %s\r\n", userName.c_str());
        //客户端发送用户名到服务器端
        iResult = send(controlSock, sendBuffer.get(),
                       (int)strlen(sendBuffer.get()), 0);
        if (iResult == SOCKET_ERROR)
            return LoginToServerRes::SEND_FAILED;
        //客户端接收服务器的响应码和信息
        //正常为"331 User name okay, need password."
        iResult = utils::recv_all(controlSock, recvMsg);
        if (iResult <= 0)
            return LoginToServerRes::RECV_FAILED;
        //检查返回码是否为331
        if (!std::regex_search(recvMsg, std::regex(R"(331.*)")))
        {
            errorMsg = std::move(recvMsg);
            return LoginToServerRes::FAILED_WITH_MSG;
        }

        // 命令 "PASS password\r\n"
        sprintf(sendBuffer.get(), "PASS %s\r\n", password.c_str());
        //客户端发送密码到服务器端
        iResult = send(controlSock, sendBuffer.get(),
                       (int)strlen(sendBuffer.get()), 0);
        if (iResult == SOCKET_ERROR)
            return LoginToServerRes::SEND_FAILED;
        //客户端接收服务器的响应码和信息
        //正常为"230 User logged in, proceed."
        iResult = utils::recv_all(controlSock, recvMsg);
        if (iResult <= 0)
            return LoginToServerRes::RECV_FAILED;
        //检查返回码是否为230
        if (!std::regex_search(recvMsg, std::regex(R"(^230.*)")))
        {
            errorMsg = std::move(recvMsg);
            return LoginToServerRes::FAILED_WITH_MSG;
        }

        return LoginToServerRes::SUCCEEDED;
    }

    PutPasvModeRes putServerIntoPasvMode(SOCKET controlSock, int &port,
                                         std::string &hostname,
                                         std::string &errorMsg)
    {
        const int sendBufLen = 1024;
        unique_ptr<char[]> sendBuffer(new char[sendBufLen]);
        std::string recvMsg;

        int iResult;
        //命令"PASV\r\n"
        sprintf(sendBuffer.get(), "PASV\r\n");
        //客户端告诉服务器用被动模式
        iResult = send(controlSock, sendBuffer.get(),
                       (int)strlen(sendBuffer.get()), 0);
        if (iResult == SOCKET_ERROR)
            return PutPasvModeRes::SEND_FAILED;

        //客户端接收服务器的响应码和新开的端口号
        //正常为"227 Entering passive mode (h1,h2,h3,h4,p1,p2)"
        iResult = utils::recv_all(controlSock, recvMsg);
        if (iResult <= 0)
            return PutPasvModeRes::RECV_FAILED;

        //检查返回码是否为227
        if (!std::regex_search(
                recvMsg, std::regex(R"(^227.*\(\d+,\d+,\d+,\d+,\d+,\d+\))")))
        {
            //返回码为500，必须要用EPSV模式
            if (std::regex_search(recvMsg, std::regex(R"(^500.*)")))
                return PutPasvModeRes::HAVE_TO_USE_EPSV;
            else
            {
                errorMsg = std::move(recvMsg);
                return PutPasvModeRes::FAILED_WITH_MSG;
            }
        }

        std::tie(hostname, port) = utils::getIPAndPortForPSAV(recvMsg);

        return PutPasvModeRes::SUCCEEDED;
    }

    PutEpsvModeRes putServerIntoEpsvMode(SOCKET controlSock, int &port,
                                         std::string &errorMsg)
    {
        const int sendBufLen = 1024;
        unique_ptr<char[]> sendBuffer(new char[sendBufLen]);
        std::string recvMsg;

        int iResult;
        //命令"EPSV\r\n"
        sprintf(sendBuffer.get(), "EPSV\r\n");
        //客户端告诉服务器用被动模式
        iResult = send(controlSock, sendBuffer.get(),
                       (int)strlen(sendBuffer.get()), 0);
        if (iResult == SOCKET_ERROR)
            return PutEpsvModeRes::SEND_FAILED;

        //客户端接收服务器的响应码和新开的端口号
        //正常为"229 Entering Extended Passive Mode (|||port|)"
        iResult = utils::recv_all(controlSock, recvMsg);
        if (iResult <= 0)
            return PutEpsvModeRes::RECV_FAILED;

        //检查返回码是否为229
        if (!std::regex_search(recvMsg, std::regex(R"(^229.*\(\|\|\|\d+\|\))")))
        {
            errorMsg = std::move(recvMsg);
            return PutEpsvModeRes::FAILED_WITH_MSG;
        }

        port = utils::getPortForEPSV(recvMsg);

        return PutEpsvModeRes::SUCCEEDED;
    }

    RequestToUpRes requestToUploadToServer(SOCKET controlSock,
                                           const std::string &remoteFileName,
                                           std::string &errorMsg)
    {
        const int sendBufLen = 1024;
        unique_ptr<char[]> sendBuffer(new char[sendBufLen]);
        std::string recvMsg;

        int iResult;
        //命令"STOR filename\r\n"
        sprintf(sendBuffer.get(), "STOR %s\r\n", remoteFileName.c_str());
        //客户端发送命令请求上传文件到服务器端
        iResult = send(controlSock, sendBuffer.get(),
                       (int)strlen(sendBuffer.get()), 0);
        if (iResult == SOCKET_ERROR)
            return RequestToUpRes::SEND_FAILED;

        //客户端接收服务器的响应码和信息
        //正常为"150 Opening data connection."
        iResult = utils::recv_all(controlSock, recvMsg);
        if (iResult <= 0)
            return RequestToUpRes::RECV_FAILED;
        //检查返回码是否为150
        if (!std::regex_search(recvMsg, std::regex(R"(^150.*)")))
        {
            errorMsg = std::move(recvMsg);
            return RequestToUpRes::FAILED_WITH_MSG;
        }

        return RequestToUpRes::SUCCEEDED;
    }

    UploadFileDataRes uploadFileDataToServer(SOCKET dataSock,
                                             std::ifstream &ifs)
    {
        if (!ifs.is_open())
            return UploadFileDataRes::READ_FILE_ERROR;

        const int sendBufLen = 1024;
        unique_ptr<char[]> sendBuffer(new char[sendBufLen]);
        std::string recvMsg;
        int iResult;
        //开始上传文件
        while (!ifs.eof())
        {
            //客户端读文件，读取一块
            ifs.read(sendBuffer.get(), sendBufLen);
            iResult = send(dataSock, sendBuffer.get(), ifs.gcount(), 0);
            if (iResult == SOCKET_ERROR)
                return UploadFileDataRes::SEND_FAILED;
        }

        return UploadFileDataRes::SUCCEEDED;
    }

} // namespace ftpclient
