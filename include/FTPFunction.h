//用于实现 FTP 协议的全局函数
//都是阻塞式函数，应当在子线程中执行
#ifndef FTP_FUNCTION_H
#define FTP_FUNCTION_H

#include <WinSock2.h>
#include <fstream>
#include <regex>
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
     * @param hostname 主机名
     * @param port 端口号，形如"21"或"ftp"均可
     * @param sendTimeout 阻塞式send()超时时间(ms)，负数表示不设置
     * @param recvTimeout 阻塞式recv()超时时间(ms)，负数表示不设置
     * @return 结果状态码
     */
    ConnectToServerRes connectToServer(SOCKET &sock,
                                       const std::string &hostname,
                                       const std::string &port, int sendTimeout,
                                       int recvTimeout);

    enum class RecvMultRes
    {
        SUCCEEDED,
        FAILED_WITH_MSG,
        FAILED
    };

    /**
     * @brief 一次性收取多条消息
     * @author zhb
     * @param controlSock 控制连接
     * @param matchRegex 用于匹配的正则
     * @param msg 出口参数，收到的消息
     * @return 结果状态码
     */
    RecvMultRes recvMultipleMsg(SOCKET controlSock,
                                const std::regex &matchRegex, std::string &msg);

    /**
     * @brief 收取多条欢迎消息
     * @author zhb
     */
    RecvMultRes recvWelcomMsg(SOCKET controlSock, std::string &msg);

    /**
     * @brief 登录成功后收取多条消息
     * @author zhb
     */
    RecvMultRes recvLoginSucceededMsg(SOCKET controlSock, std::string &msg);

    enum class CmdToServerRet
    {
        SUCCEEDED,
        FAILED_WITH_MSG,
        SEND_FAILED,
        RECV_FAILED
    };

    /**
     * @brief 向服务器发送命令并收取回复
     * @author zhb
     * @param controlSock 控制连接
     * @param sendCmd 命令，必须以"\r\n"结尾
     * @param matchRegex 匹配服务器消息的正则
     * @param recvMsg 出口参数，收到的消息
     * @return 结果状态码
     */
    CmdToServerRet cmdToServer(SOCKET controlSock, const std::string &sendCmd,
                               const std::regex &matchRegex,
                               std::string &recvMsg);

    /**
     * @brief 连接到服务器并登录
     * @author zhb
     * @param controlSock 控制连接
     * @param username 用户名
     * @param password 密码
     * @param errorMsg 出口参数，来自服务器的错误信息
     * @return 结果状态码
     */
    CmdToServerRet loginToServer(SOCKET controlSock,
                                 const std::string &username,
                                 const std::string &password,
                                 std::string &errorMsg);

    /**
     * @brief 让服务器进入PASV模式
     * @author zhb
     * @param controlSock 控制连接
     * @param hostname 出口参数，数据连接的主机名（IPv4地址）
     * @param port 出口参数，数据连接的端口号
     * @param errorMsg 出口参数，来自服务器的错误信息
     * @return 结果状态码
     */
    CmdToServerRet putServerIntoPasvMode(SOCKET controlSock, int &port,
                                         std::string &hostname,
                                         std::string &errorMsg);

    /**
     * @brief 让服务器进入EPSV模式
     * @author zhb
     * @param controlSock 控制连接
     * @param port 出口参数，数据连接的端口号
     * @param errorMsg 出口参数，来自服务器的错误信息
     * @return 结果状态码
     */
    CmdToServerRet putServerIntoEpsvMode(SOCKET controlSock, int &port,
                                         std::string &errorMsg);

    /**
     * @brief 向服务器发送 STOR 或 APPE 命令，请求上传文件
     * @author zhb
     * @param controlSock 控制连接
     * @param isAppend 是否为 APPE
     * @param remoteFilepath 服务器文件路径
     * @param errorMsg 出口参数，来自服务器的错误消息
     * @return 结果状态码
     */
    CmdToServerRet requestToUploadToServer(SOCKET controlSock, bool isAppend,
                                           const std::string &remoteFilepath,
                                           std::string &errorMsg);

    /**
     * @brief 向服务器发送 RETR 命令，请求下载文件
     * @author zhb
     * @param controlSock 控制连接
     * @param remoteFilepath 服务器文件路径
     * @param errorMsg 出口参数，来自服务器的错误消息
     * @return 结果状态码
     */
    CmdToServerRet requestRetrFromFromServer(SOCKET controlSock,
                                             const std::string &remoteFilepath,
                                             std::string &errorMsg);

    /**
     * @brief 向服务器发送 REST 命令，请求下载断点续传
     * @author zhb
     * @param controlSock 控制连接
     * @param offset 偏移量，单位字节
     * @param errorMsg 出口参数，来自服务器的错误消息
     * @return 结果状态码
     */
    CmdToServerRet requestRestFromServer(SOCKET controlSock, long long offset,
                                         std::string &errorMsg);

    /**
     * @brief 获取服务器上某个文件的大小
     * @author zhb
     * @param controlSock 控制连接
     * @param filename 服务器上的文件名
     * @param filesize 出口参数，文件大小
     * @param errorMsg 出口参数，来自服务器的错误消息
     * @return 结果状态码
     */
    CmdToServerRet getFilesizeOnServer(SOCKET controlSock,
                                       const std::string &filename,
                                       long long &filesize,
                                       std::string &errorMsg);

    /**
     * @brief 获取工作目录
     * @author zhb
     * @param controlSock 控制连接
     * @param dir 出口参数，当前工作目录
     * @param errorMsg 出口参数，来自服务器的错误消息
     * @return 结果状态码
     */
    CmdToServerRet getWorkingDirectory(SOCKET controlSock, std::string &dir,
                                       std::string &errorMsg);

    /**
     * @brief 改变工作目录
     * @author zhb
     * @param controlSock 控制连接
     * @param dir 要设定的目录
     * @param errorMsg 出口参数，来自服务器的错误消息
     * @return 结果状态码
     */
    CmdToServerRet changeWorkingDirectory(SOCKET controlSock,
                                          const std::string &dir,
                                          std::string &errorMsg);

    /**
     * @brief 设置传输模式
     * @author zhb
     * @param controlSock 控制连接
     * @param binaryMode 若为true则设为Binary模式，否则设为ASCII模式
     * @param errorMsg 出口参数，来自服务器的错误信息
     * @return 结果状态码
     */
    CmdToServerRet setBinaryOrAsciiTransferMode(SOCKET controlSock,
                                                bool binaryMode,
                                                std::string &errorMsg);

    /**
     * @brief 向服务器发 NOOP 命令
     * @author zhb
     * @param controlSock 控制连接
     * @param errorMsg 出口参数，来自服务器的错误信息
     * @return 结果状态码
     */
    CmdToServerRet sendNoopToServer(SOCKET controlSock, std::string &errorMsg);

    /**
     * @brief 删除服务器上的文件
     * @author zhb
     * @param controlSock 控制连接
     * @param filename 文件名
     * @param errorMsg 出口参数，来自服务器的错误信息
     * @return 结果状态码
     */
    CmdToServerRet deleteFileOnServer(SOCKET controlSock,
                                      const std::string &filename,
                                      std::string &errorMsg);

    /**
     * @brief 在服务器上创建目录
     * @author zhb
     * @param controlSock 控制连接
     * @param dir 目录名
     * @param errorMsg 出口参数，来自服务器的错误信息
     * @return 结果状态码
     */
    CmdToServerRet makeDirectoryOnServer(SOCKET controlSock,
                                         const std::string &dir,
                                         std::string &errorMsg);

    /**
     * @brief 移除服务器上的目录
     * @author zhb
     * @param controlSock 控制连接
     * @param dir 目录名
     * @param errorMsg 出口参数，来自服务器的错误信息
     * @return 结果状态码
     */
    CmdToServerRet removeDirectoryOnServer(SOCKET controlSock,
                                           const std::string &dir,
                                           std::string &errorMsg);

    /**
     * @brief 重命名服务器上的文件
     * @author zhb
     * @param controlSock 控制连接
     * @param oldName 当前文件名
     * @param newName 新文件名
     * @param errorMsg 出口参数，来自服务器的错误信息
     * @return 结果状态码
     */
    CmdToServerRet renameFileOnServer(SOCKET controlSock,
                                      const std::string &oldName,
                                      const std::string &newName,
                                      std::string &errorMsg);

    /**
     * @brief 向服务器发 LIST 或 NLST 命令，请求获取目录文件
     * @author zhb
     * @param controlSock 控制连接
     * @param dir 目录名
     * @param isNameList 只获取文件名
     * @param errorMsg 出口参数，来自服务器的错误信息
     * @return 结果状态码
     */
    CmdToServerRet requestToListOnServer(SOCKET controlSock,
                                         const std::string &dir,
                                         bool isNameList,
                                         std::string &errorMsg);

    enum class UploadFileDataRes
    {
        SUCCEEDED,
        SEND_FAILED,
        READ_FILE_ERROR
    };

    /**
     * @brief 将文件数据上传到服务器
     * @author zhb
     * @param dataSock 数据连接
     * @param ifs 文件输入流
     * @param percent 出口参数，已上传百分比
     * @return 结果状态码
     */
    UploadFileDataRes uploadFileDataToServer(SOCKET dataSock,
                                             std::ifstream &ifs, int &percent);

    enum class DownloadFileDataRes
    {
        SUCCEEDED,
        GET_SIZE_FAILED,
        RECV_FAILED,
        READ_FILE_ERROR
    };

    /**
     * @brief 下载服务器文件
     * @author zhb
     * @param dataSock 数据连接
     * @param ofs 文件输出流
     * @param remoteFilesize 服务器上文件大小
     * @param percent 出口参数，已下载百分比
     * @return 结果状态码
     */
    DownloadFileDataRes downloadFileDataFromServer(SOCKET dataSock,
                                                   std::ofstream &ofs,
                                                   long long remoteFilesize,
                                                   int &percent);

} // namespace ftpclient

#endif // FTP_FUNCTION_H
