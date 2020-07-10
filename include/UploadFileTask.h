#ifndef UPLOADFILETASK_H
#define UPLOADFILETASK_H

#include "../include/FTPSession.h"
#include <QObject>
#include <string>
#include <fstream>

namespace ftpclient
{

class UploadFileTask : public QObject
{
    Q_OBJECT
public:
    UploadFileTask(FTPSession &session, std::string remoteFileName, std::ifstream &ifs);
    void start();

signals:
    void uploadStarted();
    /**
     * @brief 上传成功
     */
    void uploadSucceeded();
    /**
     * @brief 上传失败
     */
    void uploadFailed();
    void uploadFailedWithMsg(std::string msg);


private:
    void dataConnect(int port);
    void startUploading();

    FTPSession &session;
    std::string remoteFileName;
    std::ifstream &ifs;
    SOCKET dataSock;
};

}//namespace ftpclient

#endif // UPLOADFILETASK_H
