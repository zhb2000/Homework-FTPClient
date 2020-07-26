#include "../include/DownloadFileTask.h"
#include "../include/FTPFunction.h"
#include "../include/MyUtils.h"
#include <QApplication>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <fstream>
#include <regex>
#include <io.h>
#include <iostream>
#include <algorithm>

namespace ftpclient
{
    DownloadFileTask::DownloadFileTask(FTPSession &session,
                                       const std::string &localFilepath,
                                       const std::string &remoteFilepath)
        : session(session.getHostname(), session.getUsername(),
                  session.getPassword(), session.getPort(), false),
          localFilepath(localFilepath),
          remoteFilepath(remoteFilepath),
          dataSocket(INVALID_SOCKET),
          controlSocket(session.getControlSock())
    {
        this->connectSignals();
        filename=QFile(QString::fromStdString(remoteFilepath)).fileName().toStdString();
    }

    DownloadFileTask::~DownloadFileTask()
    {
        this->quit();
        WSACleanup();
    }

    void DownloadFileTask::connectSignals()
    {
        QObject::connect(&session, &FTPSession::connectFailedWithMsg,
                         [this](std::string msg){
                             emit downloadFailedWithMsg(std::move(msg));
        });
        QObject::connect(&session, &FTPSession::connectFailed,
                         [this](){emit downloadFailed();});
        QObject::connect(&session, &FTPSession::createSocketFailed,
                         [this](){emit downloadFailed();});
        QObject::connect(&session, &FTPSession::unableToConnectToServer,
                         [this](){emit downloadFailed();});
        QObject::connect(&session, &FTPSession::loginFailedWithMsg,
                         [this](std::string msg){
                             emit downloadFailedWithMsg(std::move(msg));
        });
        QObject::connect(&session, &FTPSession::loginFailed,
                         [this](){emit downloadFailed();});
        QObject::connect(&session, &FTPSession::loginFailedWithMsg,
                         [this](std::string msg){emit downloadFailedWithMsg(std::move(msg));
                         });
        QObject::connect(&session, &FTPSession::setTransferModeFailed,
                         [this](){emit downloadFailed();});
        QObject::connect(&session, &FTPSession::setTransferModeSucceeded,
                         [this](){
                            if(isStop)this->prepare();
                            else this->prepare();});

        QObject::connect(&session, &FTPSession::getFilesizeFailed,
                         [this]() { emit downloadFailed(); });
        QObject::connect(&session, &FTPSession::getFilesizeFailedWithMsg,
                         [this](std::string msg) {
                             emit downloadFailedWithMsg(std::move(msg));
                         });
        //文件大小获取成功，按照既定流程进行
        QObject::connect(&session, &FTPSession::getFilesizeSucceeded,
                         [this](long long filesize) {
                             this->filesize=filesize;
                             this->prepare();
                         });
    }

    void DownloadFileTask::prepare()
    {
        qDebug()<< "prepare";
        std::string errorMsg;
        std::string hostname=session.getHostname();

        auto Msg=putServerIntoPasvMode(controlSocket, dataPort,
                              hostname, errorMsg);

        if(Msg == CmdToServerRet::SUCCEEDED)
        {
            this->dataSocketConnect();
        }
        else emit downloadFailedWithMsg(errorMsg);
    }

    void DownloadFileTask::quit()
    {
        closesocket(dataSocket);
        session.quit();
    }

    void DownloadFileTask::stop()
    {
        isStop=true;
        this->quit();
    }

    void DownloadFileTask::hasDownloaded()
    {
        std::ifstream ifs;
        ifs.open(localFilepath, std::ios::binary);
        if(ifs){
            ifs.seekg(0, ifs.end);
            int length = ifs.tellg();
            downloadOffset = length >0 ? length : 0;
            ifs.close();
         }
        else emit readFileError();
    }

    void DownloadFileTask::sendCmd(std::string cmd) {
        cmd += "\r\n";
        int cmdlen = cmd.size();
        std::cout << cmd +"\n";
        qDebug()<< cmd.data();
        send(controlSocket, cmd.c_str(), cmdlen, 0);
    }

    void DownloadFileTask::resume()
    {
        qDebug()<< "resume";
        hasDownloaded();
        isStop=false;
        session.connectAndLogin();
    }

    void DownloadFileTask::downloadFileData()
    {
         qDebug()<< "downloadFileData";
         std::ofstream ofile;
         long long hasDownloadedSize=downloadOffset;
         std::string msg;
         std::string errorMsg;

         getFilesizeOnServer(controlSocket, filename, filesize, errorMsg);
         ofile.open(localFilepath, std::ios::binary | std::ios::app);

         sendCmd("REST "+std::to_string(hasDownloadedSize));
         utils::recvFtpMsg(controlSocket, msg);//350
         sendCmd("RETR "+filename);
         utils::recvFtpMsg(controlSocket, msg);

         memset(dataBuffer, 0, DATABUFFERLEN);
         int recvData = recv(dataSocket, dataBuffer, DATABUFFERLEN, 0);
         //从recvData中获取数据传上去
         while(recvData>0)
         {
             emit downloadStarted();

             if(isStop) {
                 closesocket(dataSocket);
                 utils::recvFtpMsg(controlSocket, msg);//426
                 ofile.close();
                 emit downloadFailedWithMsg(msg);
                 return;
             }
             ofile.write(dataBuffer, recvData);
             recvData = recv(dataSocket, dataBuffer, DATABUFFERLEN, 0);
             hasDownloadedSize += recvData;
             int percent=(hasDownloadedSize * 100/filesize);
             if(percent!=100) emit percentSync(percent);
          }
          ofile.close();
          closesocket(dataSocket);
          utils::recvFtpMsg(controlSocket, msg);//226
          emit downloadSucceed();
     }

    void DownloadFileTask::dataSocketConnect()
    {
        qDebug()<< "dataSocketConnect";
        if(isStop) downloadFileData();

        auto res=connectToServer(dataSocket, session.getHostname(), std::to_string(dataPort),
                        DownloadFileTask::SENDTIMEOUT, DownloadFileTask::RECVTIMEOUT);

        if(res!=ConnectToServerRes::SUCCEEDED)emit downloadFailed();
        else downloadFileData();
    }

    void DownloadFileTask::start()
    {
        session.connectAndLogin();
    }
}
