#ifndef DOWNLOADFILETASK_H
#define DOWNLOADFILETASK_H

#include "../include/FTPSession.h"
#include <QObject>
#include <fstream>
#include <cstring>
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
         * @brief 下载前的准备工作，进入被动模式
         * @author zyc
         */
        void prepare();

        /**
         * @brief 退出下载
         * @author zyc
         */
        void quit();

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

        /**
         * @brief 连接信号
         * @author zyc
         */
        void connectSignals();

        /**
         * @brief 获取已下载文件的大小
         * @author zyc
         */
        void hasDownloaded();

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
         * @brief 发送命令
         * @param cmd 发送命令
         * @author zyc
         */
        void sendCmd(std::string cmd);

        /**
         * @brief 数据套接字连接
         * @author zyc
         */
        void dataSocketConnect();

        FTPSession session;

        std::string localFilepath;
        std::string remoteFilepath;

        SOCKET dataSocket;
        SOCKET controlSocket;

        std::string filename;

        bool isStop=false;

        const int SENDTIMEOUT=3000;
        const int RECVTIMEOUT=3000;

        const int DATABUFFERLEN=1024;

        long long downloadOffset=0;
        long long filesize=0;

        int dataPort=0;

        char *dataBuffer=new char [DATABUFFERLEN];
     };

} // namespace ftpclient

#endif // DOWNLOADFILETASK_H
