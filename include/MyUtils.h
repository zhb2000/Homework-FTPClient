#ifndef MYUTILS_H
#define MYUTILS_H

#include <string>
#include <utility>
#include <winsock2.h>

namespace utils
{

    /**
     * @brief 从"227 Entering passive mode (h1,h2,h3,h4,p1,p2)"中提取IP和端口
     * @param msg 服务器返回的信息
     * @return (IP, port)
     */
    std::pair<std::string, int> getIPAndPortForPSAV(const std::string &msg);

    // int recv_all(SOCKET sock, char *buf, int maxlen);

    int recv_all(SOCKET sock, std::string &recvMsg);

} // namespace utils

#endif // MYUTILS_H
