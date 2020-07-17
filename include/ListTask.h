#ifndef LISTTASK_H
#define LISTTASK_H

#include "../include/FTPSession.h"
#include <string>
#include <vector>

namespace ftpclient
{
    class ListTask
    {
    public:
        ListTask(FTPSession &session, const std::string &dir);
        ~ListTask()
        {
            if (dataSock != INVALID_SOCKET)
                closesocket(dataSock);
        }
        //禁止复制
        ListTask(const ListTask &) = delete;
        ListTask &operator=(const ListTask &) = delete;

        enum class Res
        {
            SUCCEEDED,
            FAILED_WITH_MSG,
            FAILED
        };

        /**
         * @brief 获取服务器返回信息的原始版本
         * @author zhb
         * @param listStrings 出口参数，每个元素为一条代表文件信息的字符串
         * @param errorMsg 出口参数，来自服务器的错误消息
         * @return 结果状态码
         */
        Res getListStrings(std::vector<std::string> &listStrings,
                           std::string &errorMsg);

    private:
        Res start(std::string &errorMsg);

        /**
         * @brief 让服务器切换到被动模式（PASV或EPSV）
         * @author zhb
         * @param errorMsg 出口参数，来自服务器的错误消息
         * @return 结果状态码
         *
         * 若成功，转到 dataConnect()
         */
        Res enterPassiveMode(std::string &errorMsg);

        /**
         * @brief 建立数据连接
         * @author zhb
         * @param hostname 数据连接主机名
         * @param port 数据连接端口号
         * @param errorMsg 出口参数，来自服务器的错误消息
         * @return 结果状态码
         *
         * 若成功，转到 requestForList()
         */
        Res dataConnect(const std::string &hostname, int port,
                        std::string &errorMsg);

        /**
         * @brief 向服务器发 LIST 命令
         * @author zhb
         * @param errorMsg
         * @return 结果状态码
         *
         * 若成功，转到 getListRawData()
         */
        Res requestForList(std::string &errorMsg);

        /**
         * @brief 通过数据连接传输数据
         * @author zhb
         * @param errorMsg 出口参数，来自服务器的错误消息
         * @return 结果状态码
         *
         * 若成功，转到 recvMsgAfterTransfer()
         */
        Res getListRawData(std::string &errorMsg);

        /**
         * @brief 传输结束后接收服务器消息
         * @author zhb
         * @param errorMsg 出口参数，来自服务器的错误消息
         * @return 结果状态码
         */
        Res recvMsgAfterTransfer(std::string &errorMsg);

        FTPSession &session;
        std::string dir;
        //若 socket 未创建，则为 INVALID_SOCKET
        SOCKET dataSock;
        //数据连接是否建立
        bool isConnect;
        //服务器返回的字符串
        std::string recvListString;

        static const int SOCKET_SEND_TIMEOUT = 1000;
        static const int SOCKET_RECV_TIMEOUT = 1000;
    };
} // namespace ftpclient

#endif // LISTTASK_H
