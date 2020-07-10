#include "../include/FTPSession.h"
#include "../include/FTPFunction.h"
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QApplication>
#include <memory>

using std::unique_ptr;

namespace ftpclient
{

FTPSession::FTPSession()
{
    controlSock = INVALID_SOCKET;
    isConnectToServer = false;
}

FTPSession::~FTPSession()
{
    if (controlSock != INVALID_SOCKET)
        closesocket(controlSock);
    WSACleanup();
}

void FTPSession::connect(const std::string &hostName, int port)
{
    this->hostName = hostName;
    //连接并登录服务器
    QFuture<ConnectToServerRes> future = QtConcurrent::run(
                [&]() { return connectToServer(controlSock,hostName,std::to_string(port)); });
    while(!future.isFinished())
        QApplication::processEvents();
    if(future.result() != ConnectToServerRes::SUCCEEDED)
    {
        if(future.result() == ConnectToServerRes::UNABLE_TO_CONNECT_TO_SERVER)
        {
            //无法连接到服务器
            emit unableToConnectToServer();
            return;
        }
        else
        {
            //创建socket失败
            emit createSocketFailed(future.result());
            return;
        }
    }
    else
        isConnectToServer = true;

    //接收服务器端的一些欢迎信息
    const int bufferlen = 512;
    unique_ptr<char[]> recvBuffer(new char[bufferlen]);
    QFuture<int> recvRes = QtConcurrent::run(
                [&]() { return recv(controlSock, recvBuffer.get(), bufferlen, 0); });
    while(!future.isFinished())
        QApplication::processEvents();
    if (recvRes.result() > 0)
    {
        int len = recvRes.result();
        //成功
        emit connectionToServerSucceeded(std::string(recvBuffer.get(), len));
    }
    else if (recvRes.result() == 0)
    {
        //服务器已关闭连接
        emit unableToConnectToServer();
        isConnectToServer = false;
    }
    else
    {
        //recv失败
        emit recvFailed();
        isConnectToServer = false;
    }
}

void FTPSession::login(const std::string &userName, const std::string &password)
{
    std::string errorMsg;
    QFuture<LoginToServerRes> future = QtConcurrent::run(
                [&]() { return loginToServer(controlSock, userName, password, errorMsg); });
    while (!future.isFinished())
        QApplication::processEvents();
    auto res = future.result();
    if (res == LoginToServerRes::SUCCEEDED)
        emit loginSucceeded();
    else if (res == LoginToServerRes::LOGIN_FAILED)
        emit loginFailed(std::move(errorMsg));
    else if (res == LoginToServerRes::SEND_FAILED)
        emit sendFailed();
    else if (res == LoginToServerRes::RECV_FAILED)
        emit recvFailed();
}

void FTPSession::getPasvDataPort()
{
    int port;
    std::string errorMsg;
    QFuture<PutPasvModeRes> future = QtConcurrent::run(
                [&]() { return putServerIntoPasvMode(controlSock, port, errorMsg); });
    while (!future.isFinished())
        QApplication::processEvents();
    if (future.result() == PutPasvModeRes::SUCCEEDED)
        emit putPasvSucceeded(port);
    else if (future.result() == PutPasvModeRes::FAILED)
        emit putPasvFailed(std::move(errorMsg));
    else if (future.result() == PutPasvModeRes::SEND_FAILED)
        emit sendFailed();
    else if (future.result() == PutPasvModeRes::RECV_FAILED)
        emit recvFailed();
}

void FTPSession::close()
{
    //TODO(zhb) 尚未完成
}

}//namespace ftpclient
