//一些实用工具
#ifndef MYUTILS_H
#define MYUTILS_H

#include <fstream>
#include <string>
#include <utility>
#include <vector>
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
    long long getSizeFromMsg(const std::string &msg);

    /**
     * @brief 从 '"dir"' 中提取目录
     * @author zhb
     * @param msg 服务器返回的消息
     * @return 表示目录的字符串
     */
    std::string getDirFromMsg(const std::string &msg);

    /**
     * @brief 不断收取数据，直到对方关闭连接
     * @author zhb
     * @param sock
     * @param recvMsg 出口参数，收到的消息
     * @return 收到的字节数，负数表示出错
     */
    int recvUntilClose(SOCKET sock, std::string &recvMsg);

    /**
     * @brief 接收一条消息（以"\r\n"结尾）
     * @author zhb
     * @param controlSock 控制连接
     * @param recvMsg 出口参数，收到的消息
     * @return 收到的字节数，负数表示出错
     */
    int recvFtpMsg(SOCKET controlSock, std::string &recvMsg);

    /**
     * @brief 获取文件的大小（字节）
     * @author zhb
     * @param ifs 文件输入流
     * @return 文件大小（字节）
     */
    long long getFilesize(std::ifstream &ifs);

    /**
     * @brief 将文本按行分割
     * @author zhb
     * @param text 文本
     * @return vector中的每个元素为原文本中的一行
     */
    std::vector<std::string> splitLines(const std::string &text);

} // namespace utils

#endif // MYUTILS_H
