#ifndef FTPSESSION_H
#define FTPSESSION_H

#include "../include/FTPFunction.h"
#include <QObject>
#include <functional>
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
         * @author zhb
         * @param filename 服务器上的文件名
         *
         * 异步函数，运行结束后会发射以下信号之一：
         * - getFilesizeSucceeded
         * - getFilesizeFailedWithMsg(msg)
         * - getFilesizeFailed
         */
        void getFilesize(const std::string &filename);

        /**
         * @brief 获取当前工作目录
         * @author zhb
         *
         * 异步函数，运行结束后会发射以下信号之一：
         * - getDirSucceeded(dir);
         * - getDirFailedWithMsg(msg);
         * - getDirFailed
         */
        void getDir();

        /**
         * @brief 改变工作目录
         * @author zhb
         * @param dir 要设置的工作目录
         *
         * 异步函数，运行结束后会发射以下信号之一：
         * - changeDirSucceeded(dir);
         * - changeDirFailedWithMsg(msg);
         * - changeDirFailed
         */
        void changeDir(const std::string &dir);

        /**
         * @brief 设定传输模式
         * @author zhb
         * @param binaryMode 传输模式，true为Binary模式，false为ASCII模式
         *
         * 异步函数，，运行结束后会发射以下信号之一：
         * - setTransferModeSucceeded(binaryMode)
         * - setTransferModeFailedWithMsg(msg)
         * - setTransferModeFailed
         */
        void setTransferMode(bool binaryMode);

        /**
         * @brief 删除服务器上的文件
         * @author zhb
         * @param filename 文件名
         *
         * 异步函数，运行结束后会发射以下信号之一：
         * - deleteFileSucceeded
         * - deleteFileFailedWithMsg(msg)
         * - deleteFileFailed
         */
        void deleteFile(const std::string &filename);

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

        /**
         * @brief 获取当前工作目录成功
         * @param dir 工作目录
         */
        void getDirSucceeded(std::string dir);
        /**
         * @brief 获取当前工作目录失败（带错误消息）
         * @param msg 来自服务器的错误消息
         */
        void getDirFailedWithMsg(std::string msg);
        /**
         * @brief 获取当前工作目录失败
         */
        void getDirFailed();

        /**
         * @brief 改变工作目录成功
         */
        void changeDirSucceeded();
        /**
         * @brief 改变工作目录失败（带错误消息）
         * @param msg 来自服务器的错误消息
         */
        void changeDirFailedWithMsg(std::string msg);
        /**
         * @brief 改变工作目录失败
         */
        void changeDirFailed();

        /**
         * @brief 设定传输模式成功
         * @param binaryMode 传输模式，true为Binary模式，false为ASCII模式
         */
        void setTransferModeSucceeded(bool binaryMode);
        /**
         * @brief 设定传输模式失败（带错误消息）
         * @param msg 来自服务器的错误消息
         */
        void setTransferModeFailedWithMsg(std::string msg);
        /**
         * @brief 设定传输模式失败
         */
        void setTransferModeFailed();

        /**
         * @brief 删除文件成功
         */
        void deleteFileSucceeded();
        /**
         * @brief 删除文件失败（带错误消息）
         * @param msg 来自服务器的错误消息
         */
        void deleteFileFailedWithMsg(std::string msg);
        /**
         * @brief 删除文件失败
         */
        void deleteFileFailed();

        // recvFailed 和 sendFailed 信号用于 Debug
        //信号：recv()失败
        void recvFailed();
        //信号：send()失败
        void sendFailed();

    private:
        /**
         * @brief 在子线程中执行阻塞式函数，结束后发射信号
         * @param func 需要在子线程中运行的阻塞式函数
         * @param succeededSignal 成功信号
         * @param failedWithMsgSignal 失败信号（参数为错误消息）
         * @param failedSignal 失败信号
         *
         * 适用于成功信号为无参函数的情况
         *
         * 调用方法：
         * 1. 用 std::bind 绑定阻塞函数的各个参数，只留 string &errorMsg
         * 这一个出口参数
         * 2. 将绑定后的函数对象传入 func 参数
         * 3. 将各个信号的类成员函数指针传入其余三个参数
         */
        void runProcedure(std::function<CmdToServerRet(std::string &)> func,
                          void (FTPSession::*succeededSignal)(),
                          void (FTPSession::*failedWithMsgSignal)(std::string),
                          void (FTPSession::*failedSignal)());

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
