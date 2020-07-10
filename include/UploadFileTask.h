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
    void uploadFinished();
    void uploadFailed();

    void putPasvFailed(std::string errorMsg);

private:
    FTPSession &session;
    std::string remoteFileName;
    std::ifstream &ifs;

};

}//namespace ftpclient

#endif // UPLOADFILETASK_H
