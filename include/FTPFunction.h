#ifndef FTP_FUNCTION_H
#define FTP_FUNCTION_H

#include <WinSock2.h>
#include <fstream>
#include <string>

namespace ftpclient
{

    enum class ConnectToServerRes
    {
        //成功连接到服务器
        SUCCEEDED,
        WSAStartup_FAILED,
        getaddrinfo_FAILED,
        socket_FAILED,
        //无法连接服务器
        UNABLE_TO_CONNECT_TO_SERVER
    };

    /**
     * @brief 创建socket并连接到服务器
     * @author zhb
     * @param sock 出口参数，创建的socket
     * @param hostName 主机名
     * @param port 端口号，形如"21"或"ftp"均可
     * @return 结果状态码
     */
    ConnectToServerRes connectToServer(SOCKET &sock,
                                       const std::string &hostName,
                                       const std::string &port);

    enum class LoginToServerRes
    {
        //登录成功
        SUCCEEDED,
        // send()失败
        SEND_FAILED,
        // recv()失败
        RECV_FAILED,
        //登录失败
        LOGIN_FAILED
    };

    /**
     * @brief 连接到服务器并登录，该函数为阻塞式
     * @author zhb
     * @param controlSock 控制连接
     * @param userName 用户名
     * @param password 密码
     * @param errorMsg 出口参数，来自服务器的错误信息
     * @return 结果状态码
     */
    LoginToServerRes loginToServer(SOCKET controlSock,
                                   const std::string &userName,
                                   const std::string &password,
                                   std::string &errorMsg);

    enum class PutPasvModeRes
    {
        SUCCEEDED,
        FAILED,
        SEND_FAILED,
        RECV_FAILED
    };

    /**
     * @brief 让服务器进入被动模式
     * @author zhb
     * @param controlSock 控制连接
     * @param port 出口参数，数据连接的端口号
     * @param errorMsg 出口参数，来自服务器的错误信息
     * @return 结果状态码
     */
    PutPasvModeRes putServerIntoPasvMode(SOCKET controlSock, int &port,
                                         std::string &errorMsg);

    enum class UploadToServerRes
    {
        SUCCEEDED,
        FAILED_WITH_MSG,
        SEND_FAILED,
        RECV_FAILED,
        READ_FILE_ERROR
    };

    /**
     * @brief uploadFileToServer
     * @author zhb
     * @param dataSock 数据连接
     * @param remoteFileName 服务器文件名
     * @param ifs 文件输入流
     * @param errorMsg 输出参数，来自服务器的错误消息
     * @return 结果状态码
     *
     * 目前还没法暂停或停止上传
     */
    UploadToServerRes uploadFileToServer(SOCKET dataSock,
                                         const std::string &remoteFileName,
                                         std::ifstream &ifs,
                                         std::string &errorMsg);

} // namespace ftpclient

#endif // FTP_FUNCTION_H
