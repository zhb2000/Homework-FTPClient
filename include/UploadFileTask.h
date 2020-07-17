#ifndef UPLOADFILETASK_H
#define UPLOADFILETASK_H

#include "../include/FTPSession.h"
#include <QObject>
#include <fstream>
#include <string>

namespace ftpclient
{

    /**
     * @brief 上传任务类，一个FTP Session 可以对应多个上传任务
     *
     * 调用 start() 开始任务
     * 调用 stop() 停止任务
     */
    class UploadFileTask : public QObject
    {
        Q_OBJECT
    public:
        /**
         * @brief UploadFileTask 构造函数
         * @author zhb
         * @param session FTP会话
         * @param remoteFileName 服务器文件名
         * @param ifs 文件输入流
         */
        UploadFileTask(FTPSession &session, std::string remoteFileName,
                       std::ifstream &ifs);

        ~UploadFileTask();
        //禁止复制
        UploadFileTask(const UploadFileTask &) = delete;
        UploadFileTask &operator=(const UploadFileTask &) = delete;

        /**
         * @brief 开始上传任务
         * @author zhb
         */
        void start();

        /**
         * @brief 停止上传
         * @author zhb
         */
        void stop();

    signals:
        /**
         * @brief 信号：上传开始
         */
        void uploadStarted();
        /**
         * @brief 信号：上传百分比
         * @param percentage 百分比
         */
        void uploadPercentage(int percentage);
        /**
         * @brief 信号：上传成功
         */
        void uploadSucceeded();

        /**
         * @brief 信号：上传失败
         */
        void uploadFailed();
        /**
         * @brief 信号：上传失败（带错误消息的）
         * @param msg 来自服务器的错误消息
         */
        void uploadFailedWithMsg(std::string msg);
        /**
         * @brief 信号：读取文件错误
         */
        void readFileError();

    private:
        /**
         * @brief 让服务器切换到被动模式（PASV或EPSV）
         * @author zhb
         *
         * 异步函数
         * - 若成功，转到 dataConnect()
         * - 若失败，发射信号 uploadFailedWithMsg(msg) 或 uploadFailed
         */
        void enterPassiveMode();

        /**
         * @brief 与服务器建立数据连接
         * @author zhb
         * @param hostname 主机名
         * @param port 端口号
         *
         * 异步函数
         * - 若成功，转到 uploadRequest()
         * - 若失败，发射 uploadFailed 信号
         */
        void dataConnect(const std::string &hostname, int port);

        /**
         * @brief 向服务器发 STOR 命令请求上传
         * @author zhb
         *
         * 异步函数
         * - 若服务器同意上传，发射 uploadStarted 信号，随后转到执行
         * uploadFileData()，向服务器发送文件内容
         * - 若服务器拒绝上传，发射 uploadFailedWithMsg(msg) 信号
         * - 其他网络错误，发射 uploadFailed 信号
         */
        void uploadRequest();

        /**
         * @brief 向服务器发送文件内容
         * @author zhb
         *
         * 异步函数，运行结束后会发射以下信号之一
         * - uploadSucceeded
         * - uploadFailed
         * - readFileError
         */
        void uploadFileData();

        void connectSessionSignals();

        /**
         * @brief 关闭控制连接和数据连接
         */
        void quit();

        //需要额外建一个控制连接
        FTPSession session;
        //文件名
        std::string remoteFileName;
        //文件输入流
        std::ifstream &ifs;
        //若 socket 未创建，则为 INVALID_SOCKET
        SOCKET dataSock;
        //数据连接是否建立
        bool isDataConnected;
        // stop()是否被调用
        bool isSetStop;

        static const int SOCKET_SEND_TIMEOUT = 3000;
        static const int SOCKET_RECV_TIMEOUT = 3000;
    };

} // namespace ftpclient

#endif // UPLOADFILETASK_H
