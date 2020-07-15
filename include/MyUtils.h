#ifndef MYUTILS_H
#define MYUTILS_H

#include <string>
#include <utility>
#include <winsock2.h>

namespace utils
{

    /**
     * @brief 从"227 Entering passive mode (h1,h2,h3,h4,p1,p2)"中提取IP和端口
     * @author zhb
     * @param msg 服务器返回的信息
     * @return (IP, port)
     */
    std::pair<std::string, int> getIPAndPortForPSAV(const std::string &msg);

    /**
     * @brief 从"229 Entering Extended Passive Mode (|||port|)"中提取端口号
     * @author zhb
     * @param msg 服务器返回的信息
     * @return 端口号
     */
    int getPortForEPSV(const std::string &msg);

    /**
     * @brief 从"213 size"中提取文件大小
     * @author zhb
     * @param msg 服务器返回的消息
     * @return 文件大小
     */
    int getSizeFromMsg(const std::string &msg);

    /**
     * @brief 从 '"dir"' 中提取目录
     * @author zhb
     * @param msg 服务器返回的消息
     * @return 表示目录的字符串
     */
    std::string getDirFromMsg(const std::string &msg);

    /**
     * @brief 多次调用recv()直到收完所有数据
     * @author zhb
     * @param sock
     * @param recvMsg 出口参数，收到的消息
     * @return 收到的字节数，负数表示出错
     */
    int recv_all(SOCKET sock, std::string &recvMsg);

} // namespace utils

#endif // MYUTILS_H
