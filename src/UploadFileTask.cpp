#include "../include/UploadFileTask.h"
#include "../include/FTPFunction.h"
#include <QFuture>
#include <QtConcurrent/QtConcurrent>

namespace ftpclient
{

UploadFileTask::UploadFileTask(FTPSession &session, std::string fileName, std::ifstream &ifs)
    : session(session),
      remoteFileName(std::move(fileName)),
      ifs(ifs) {}

void UploadFileTask::start()
{
    QObject::connect(&session, &FTPSession::putPasvFailed,
                     [&](std::string errorMsg)
    { emit putPasvFailed(std::move(errorMsg)); emit uploadFailed(); });
    QObject::connect(&session, &FTPSession::sendFailed,
                     [&]() { emit uploadFailed(); });
    QObject::connect(&session, &FTPSession::recvFailed,
                     [&]() { emit uploadFailed(); });
    QObject::connect(&session, &FTPSession::putPasvSucceeded,
                     [&](int port)
    {
        //TODO(zhb) 尚未完成
    });

    session.getPasvDataPort();
}

}//namespace ftpclient
