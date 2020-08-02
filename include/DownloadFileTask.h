#ifndef DOWNLOADFILETASK_H
#define DOWNLOADFILETASK_H

#include "../include/FTPSession.h"
#include <QObject>
#include <cstring>
#include <fstream>
#include <iostream>

namespace ftpclient
{

    /**
     * @brief 下载任务类
     * 调用 start() 开始任务
     * 调用 resume() 继续任务
     * 调用 stop() 停止任务
     */
    class DownloadFileTask : public QObject
    {
        Q_OBJECT
    public:
        /**
         * @brief DownloadFileTask 构造函数
         * @author zyc
         * @param session FTP会话
         * @param localFilepath 本地文件路径，应使用绝对路径
         * @param remoteFilepath 服务器文件路径，应使用绝对路径
         */
        DownloadFileTask(FTPSession &session, const std::string &localFilepath,
                         const std::string &remoteFilepath);

        ~DownloadFileTask();

        DownloadFileTask(const DownloadFileTask &) = delete;
        DownloadFileTask &operator=(const DownloadFileTask &) = delete;

        /**
         * @brief 下载文件内容
         * @author zyc
         */
        void downloadFileData();

        /**
         * @brief 开始下载
         * @author zyc
         */
        void start();

        /**
         * @brief 继续下载
         * @author zyc
         */
        void resume();

        /**
         * @brief 停止下载
         * @author zyc
         */
        void stop();

    signals:

        /**
         * @brief 信号：下载百分比
         * @param percent 百分比
         */
        void percentSync(int percent);

        /**
         * @brief 信号：下载开始
         */
        void downloadStarted();

        /**
         * @brief 信号：下载失败
         * @param msg 错误信息
         */
        void downloadFailedWithMsg(std::string msg);

        /**
         * @brief 信号：下载失败
         */
        void downloadFailed();

        /**
         * @brief 信号：下载失败
         */
        void readFileError();

        /**
         * @brief 信号：下载成功
         */
        void downloadSucceed();

    private:
        /**
         * @brief 连接信号
         * @author zyc
         */
        void connectSignals();

        /**
         * @brief 进入被动模式
         * @author zyc
         */
        void enterPassiveMode();

        /**
         * @brief 数据套接字连接
         * @author zyc
         * @param hostname 主机名
         * @param port 端口号
         */
        void dataSocketConnect(const std::string &hostname, int port);

        /**
         * @brief 发送 RETR 或 REST 命令请求下载
         */
        void downloadRequest();

        /**
         * @brief 退出下载
         * @author zyc
         */
        void quit();

        //额外建立的控制连接
        FTPSession session;
        //本地文件路径
        std::string localFilepath;
        //服务器文件路径
        std::string remoteFilepath;
        //文件输出流
        std::ofstream ofs;
        //若 socket 未创建，则为 INVALID_SOCKET
        SOCKET dataSocket;
        //数据连接是否建立
        bool isDataConnected;
        // stop()是否被调用
        bool isSetStop;
        //是否为续传
        bool isReset;
        //服务器上文件的大小
        long long remoteFilesize;
        long long downloadOffset = 0;

        static const int SENDTIMEOUT = 3000;
        static const int RECVTIMEOUT = 3000;
    };

} // namespace ftpclient

#endif // DOWNLOADFILETASK_H
