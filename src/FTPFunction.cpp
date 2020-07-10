#include "../include/FTPFunction.h"
#include "../include/ScopeGuard.h"
#include "../include/MyUtils.h"
#include <ws2tcpip.h>
#include <memory>
#include <cstdio>
#include <cstring>
#include <regex>

using utils::ScopeGuard;
using std::unique_ptr;

namespace ftpclient
{

ConnectToServerRes
connectToServer(SOCKET &sock,const std::string &hostName, const std::string &port)
{
    WSADATA wsaData;
    int iResult;
    //初始化Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
        return ConnectToServerRes::WSAStartup_FAILED;
    ScopeGuard guardCleanWSA([]() { WSACleanup(); });//WSACleanup()用于结束对WS2_32 DLL的使用

    addrinfo *result = nullptr;
    addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));//结构体初始化
    hints.ai_family = AF_UNSPEC;//未指定，IPv4和IPv6均可
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
            break;//连接服务器成功
    }
    if (sock == INVALID_SOCKET)
        return ConnectToServerRes::UNABLE_TO_CONNECT_TO_SERVER;
    else
    {
        setSendRecvTimeout(sock, 5000, 5000);
        guardCleanWSA.dismiss();
        return ConnectToServerRes::SUCCEEDED;
    }
}

bool setSendRecvTimeout(SOCKET sock, int sendTimeout, int recvTimeout)
{
    int iResult;
    iResult = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO,
                         (const char*)&sendTimeout, sizeof(sendTimeout));
    if (iResult != 0)
        return false;
    iResult = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
                         (const char*)&recvTimeout, sizeof(recvTimeout));
    if(iResult != 0)
        return false;
    return true;
}

LoginToServerRes
loginToServer(SOCKET controlSock, const std::string &userName,
              const std::string &password, std::string &errorMsg)
{

    const int sendBufLen = 1024;
    const int recvBufLen = 1024;
    unique_ptr<char[]> sendBuffer(new char[sendBufLen]);
    unique_ptr<char[]> recvBuffer(new char[recvBufLen]);

    int iResult;
    //命令 "USER username\r\n"
    sprintf(sendBuffer.get(), "USER %s\r\n", userName.c_str());
    //客户端发送用户名到服务器端
    iResult = send(controlSock, sendBuffer.get(), (int)strlen(sendBuffer.get()), 0);
    if (iResult == SOCKET_ERROR)
        return LoginToServerRes::SEND_FAILED;
    //客户端接收服务器的响应码和信息
    //正常为"331 User name okay, need password."
    iResult = recv(controlSock, recvBuffer.get(), recvBufLen, 0);
    if (iResult == SOCKET_ERROR)
        return LoginToServerRes::RECV_FAILED;
    //检查返回码是否为331
    if (!std::regex_search(recvBuffer.get(), recvBuffer.get() + iResult,
                           std::regex(R"(331.*)")))
    {
        errorMsg = std::string(recvBuffer.get(), iResult);
        return LoginToServerRes::LOGIN_FAILED;
    }


    // 命令 "PASS password\r\n"
    sprintf(sendBuffer.get(), "PASS %s\r\n", password.c_str());
    //客户端发送密码到服务器端
    iResult = send(controlSock, sendBuffer.get(), (int)strlen(sendBuffer.get()), 0);
    if (iResult == SOCKET_ERROR)
        return LoginToServerRes::SEND_FAILED;
    //客户端接收服务器的响应码和信息
    //正常为"230 User logged in, proceed."
    iResult = recv(controlSock, recvBuffer.get(), recvBufLen, 0);
    if (iResult <= 0)
        return LoginToServerRes::RECV_FAILED;
    //检查返回码是否为230
    if (!std::regex_search(recvBuffer.get(), recvBuffer.get() + iResult,
                           std::regex(R"(230.*)")))
    {
        errorMsg = std::string(recvBuffer.get(), iResult);
        return LoginToServerRes::LOGIN_FAILED;
    }

    return LoginToServerRes::SUCCEEDED;
}

PutPasvModeRes putServerIntoPasvMode(SOCKET controlSock, int &port, std::string &errorMsg)
{
    const int sendBufLen = 1024;
    const int recvBufLen = 1024;
    unique_ptr<char[]> sendBuffer(new char[sendBufLen]);
    unique_ptr<char[]> recvBuffer(new char[recvBufLen]);

    int iResult;
    //命令"PASV\r\n"
    sprintf(sendBuffer.get(), "PASV\r\n");
    //客户端告诉服务器用被动模式
    iResult = send(controlSock, sendBuffer.get(), (int)strlen(sendBuffer.get()), 0);
    if (iResult == SOCKET_ERROR)
        return PutPasvModeRes::SEND_FAILED;
    //客户端接收服务器的响应码和新开的端口号
    //正常为"227 Entering passive mode (h1,h2,h3,h4,p1,p2)"
    iResult = recv(controlSock, recvBuffer.get(), recvBufLen, 0);
    if (iResult <= 0)
        return PutPasvModeRes::RECV_FAILED;
    //检查返回码是否为227
    if (!std::regex_search(recvBuffer.get(), recvBuffer.get() + iResult,
                           std::regex(R"(227.*\(\d+,\d+,\d+,\d+,\d+,\d+\)[.\r\n]*)")))
    {
        errorMsg = std::string(recvBuffer.get(), iResult);
        return PutPasvModeRes::FAILED;
    }

    int p1, p2;
    std::tie(p1, p2) = utils::getPortFromStr(std::string(recvBuffer.get(), iResult));
    port = p1 * 256 + p2;
    return PutPasvModeRes::SUCCEEDED;
}

UploadToServerRes uploadFileToServer(SOCKET dataSock,
                                     const std::string &remoteFileName,
                                     std::ifstream &ifs,
                                     std::string &errorMsg)
{
    const int sendBufLen = 1024;
    const int recvBufLen = 1024;
    unique_ptr<char[]> sendBuffer(new char[sendBufLen]);
    unique_ptr<char[]> recvBuffer(new char[recvBufLen]);

    int iResult;
    //命令"STOR filename\r\n"
    sprintf(sendBuffer.get(), "STOR %s\r\n", remoteFileName.c_str());
    //客户端发送命令上传文件到服务器端
    iResult = send(dataSock, sendBuffer.get(), (int)strlen(sendBuffer.get()), 0);
    if (iResult == SOCKET_ERROR)
        return UploadToServerRes::SEND_FAILED;

    //客户端接收服务器的响应码和信息
    //正常为"150 Opening data connection."
    iResult = recv(dataSock, recvBuffer.get(), recvBufLen, 0);
    if (iResult <= 0)
        return UploadToServerRes::RECV_FAILED;
    //检查返回码是否为150
    if (!std::regex_search(recvBuffer.get(), recvBuffer.get() + iResult,
                           std::regex(R"(150.*)")))
    {
        errorMsg = std::string(recvBuffer.get(), iResult);
        return UploadToServerRes::FAILED;
    }

    //开始上传文件
    while (true)
    {
        //客户端读文件，读取一块
        ifs.read(sendBuffer.get(), sendBufLen);
        iResult = send(dataSock, sendBuffer.get(), ifs.gcount(), 0);
        if (iResult == SOCKET_ERROR)
            return UploadToServerRes::SEND_FAILED;
        if (ifs.rdstate() == std::ios_base::goodbit)
            continue;//无错误
        else if (ifs.rdstate() == std::ios_base::eofbit)
            break;//关联的输出序列已抵达文件尾
        else
            return UploadToServerRes::READ_FILE_ERROR;
    }
    return UploadToServerRes::SUCCEEDED;
}

}//namespace ftpclient
