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
     * @author zhb
     *
     * 调用成员函数 start() 后，应通过信号了解上传任务的状态
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
        /**
         * @brief 开始上传任务
         * @author zhb
         */
        void start();

    signals:
        /**
         * @brief 信号：上传开始
         */
        void uploadStarted();
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
         * @brief TODO(zhb) 信号：上传百分比
         * @param percentage 百分比
         */
        void uploadPercentage(int percentage);

    private:
        /**
         * @brief 与服务器建立数据连接
         * @author zhb
         * @param hostname 主机名
         * @param port 端口号
         *
         * 异步函数
         * - 若数据连接建立成功，发射 uploadStarted 信号，随后执行
         * UploadFileTask::startUploading()，向服务器发送文件内容
         * - 若数据连接建立失败，发射 uploadFailed 信号
         */
        void dataConnect(const std::string &hostname, int port);

        /**
         * @brief 向服务器发送文件内容
         * @author zhb
         *
         * 异步函数，运行结束后会发射以下信号之一
         * - uploadSucceeded
         * - uploadFailedWithMsg
         * - uploadFailed
         */
        void startUploading();

        FTPSession &session;
        std::string remoteFileName;
        std::ifstream &ifs;
        SOCKET dataSock;
    };

} // namespace ftpclient

#endif // UPLOADFILETASK_H
