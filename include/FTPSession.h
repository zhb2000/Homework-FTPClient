#ifndef FTPSESSION_H
#define FTPSESSION_H

#include "../include/FTPFunction.h"
#include <QObject>
#include <string>
#include <winsock2.h>

namespace ftpclient
{

    class FTPSession : public QObject
    {
        Q_OBJECT
    public:
        FTPSession();
        ~FTPSession();
        //禁止复制
        FTPSession(const FTPSession &) = delete;
        FTPSession &operator=(const FTPSession &) = delete;

        /**
         * @brief 创建socket、连接服务器
         * @author zhb
         * @param hostName 服务器主机名
         * @param port 端口号
         *
         * 异步函数，运行结束后会发射以下信号之一：
         * - connectionToServerSucceeded(welcomeMsg)
         * - createSocketFailed(reason)
         * - unableToConnectToServer
         * - recvFailed
         */
        void connect(const std::string &hostname, int port = 21);

        /**
         * @brief 登录FTP服务器
         * @author zhb
         * @param userName 用户名
         * @param password 密码
         *
         * 异步函数，运行结束后会发射以下信号之一：
         * - loginSucceeded
         * - loginFailed
         * - loginFailedWithMsg(msg)
         *
         * - sendFailed
         * - recvFailed
         */
        void login(const std::string &username, const std::string &password);

        /**
         * @brief 获取文件大小
         * @param filename 服务器上的文件名
         *
         * 异步函数，运行结束后会发射以下信号之一：
         * - getFilesizeSucceeded
         * - getFilesizeFailedWithMsg(msg)
         * - getFilesizeFailed
         */
        void getFilesize(const std::string &filename);

        /**
         * @brief 关闭控制端口的连接
         * @author zhb
         *
         * 尚未完成
         */
        void close();

        std::string getHostName() const { return hostname; }

        SOCKET getControlSock() const { return controlSock; }

    signals:

        /**
         * @brief 信号：连接服务器成功
         * @param welcomeMsg 来自服务器的欢迎信息
         */
        void connectionToServerSucceeded(std::string welcomeMsg);
        /**
         * @brief 信号：创建socket失败
         * @param reason 失败原因
         */
        void createSocketFailed(ConnectToServerRes reason);
        /**
         * @brief 信号：无法连接到服务器
         */
        void unableToConnectToServer();

        /**
         * @brief 信号：登录成功
         */
        void loginSucceeded();
        /**
         * @brief 信号：登录失败（带错误消息）
         * @param errorMsg 来自服务器的错误信息
         */
        void loginFailedWithMsg(std::string errorMsg);
        /**
         * @brief 信号：登录失败
         */
        void loginFailed();

        /**
         * @brief 信号：获取文件大小成功
         * @param size 文件大小
         */
        void getFilesizeSucceeded(int size);
        /**
         * @brief 信号：获取文件大小失败（带错误消息）
         * @param errorMsg 来自服务器的错误消息
         */
        void getFilesizeFailedWithMsg(std::string errorMsg);
        /**
         * @brief 信号：获取文件大小失败
         */
        void getFilesizeFailed();

        // recvFailed 和 sendFailed 信号用于 Debug
        //信号：recv()失败
        void recvFailed();
        //信号：send()失败
        void sendFailed();

    private:
        //若 socket 尚未创建，则 controlSock 为 INVALID_SOCKET
        SOCKET controlSock;
        //控制连接是否建立
        bool isConnected;
        std::string hostname;

        static const int SOCKET_SEND_TIMEOUT = 3000;
        static const int SOCKET_RECV_TIMEOUT = 3000;
    };

} // namespace ftpclient

#endif // FTPSESSION_H
