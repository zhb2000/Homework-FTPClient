#include "../include/mainwindow.h"
//#include "../include/UserInteraction.h"
#include "../include/UploadFileTask.h"
#include "ui_mainwindow.h"
#include <iostream>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QtDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>

using namespace std;
using namespace ftpclient;
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    //initialize user interface
    setFixedSize(675, 450);
    ui->setupUi(this);
    ui->progressBar->setVisible(false);
    hideFTPFunction(false);

    //initialize listView
    qml=new QStringListModel(this);
    ui->dir->setModel(qml);

    ui->displayingMsg->append("Welcome to the FTP Client designed by ZHB and ZYC.");
    ui->displayingMsg->append("Here display debug messages.");
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::hideFTPFunction(bool value)
{
    ui->startButton->setVisible(value);
    ui->download->setVisible(value);
    ui->upload->setVisible(value);
    ui->sizeButton->setVisible(value);
    ui->deleteButton->setVisible(value);
    ui->newDirButton->setVisible(value);
    ui->renameButton->setVisible(value);
    ui->transferModeButton->setVisible(value);
    ui->returnButton->setVisible(value);
}

void MainWindow::on_connectButton_clicked()
{
    if(isConnected){
        se->quit();
        ui->connectButton->setText("连接");
        hideFTPFunction(false);
    }
    else {
        QString hostname=ui->hostAddressText->toPlainText();
        QString password=ui->passwordText->text();
        QString username=ui->usernameText->toPlainText();
        this->se=new ftpclient::FTPSession(hostname.toStdString(),
                                           username.toStdString(),
                                           password.toStdString());
        this->initConnection(se);
        //login
        se->connectAndLogin();
    }
}

void MainWindow::connectUploadSignals(UploadFileTask *task)
{
    QObject::connect(task, &UploadFileTask::uploadStarted,
                     [&]() { qDebug("uploadStarted");
                             ui->startButton->setVisible(true);
                             ui->startButton->setText("暂停");
                             ui->progressBar->setTextVisible(true);
                             isUploading=true;
                             uploadCount++;
                             ui->displayingMsg->append("uploadStarted");});

    QObject::connect(task, &UploadFileTask::uploadSucceeded,
                     [&]() { qDebug("uploadSucceeded");
                             ui->displayingMsg->append("uploadSucceeded");
                             uploadCount--;
                             if(uploadCount<=0)isUploading=false;
                             if(!isUploading)ui->startButton->setVisible(false);
                             });

    QObject::connect(task, &UploadFileTask::uploadFailedWithMsg,
                     [&](std::string errorMsg) {
                         qDebug("uploadFailedWithMsg");
                         qDebug() << errorMsg.data();
                         ui->displayingMsg->append("uploadFailedWithMsg");
                         ui->displayingMsg->append(QString::fromStdString(errorMsg));
                     });

    QObject::connect(task, &UploadFileTask::uploadFailed,
                     [&]() { qDebug("uploadFailed");
                             ui->displayingMsg->append("uploadFailed");});


    QObject::connect(task, &UploadFileTask::readFileError,
                     [&]() { qDebug("readFileError");
                             ui->displayingMsg->append("readFileError");});


    QObject::connect(task, &UploadFileTask::uploadPercentage,
                     [&](int percentage) {
                         qDebug() << "percentage: " << percentage << "%";
                         ui->progressBar->setValue(percentage/uploadCount);
                     });
}

void MainWindow::initConnection(FTPSession *se)
{
    QObject::connect(se, &FTPSession::recvFailed,
                     [&]() { qDebug() << "recvFailed";
                             ui->displayingMsg->append("recvFailed");});

    QObject::connect(se, &FTPSession::sendFailed,
                     [&]() { qDebug() << "sendFailed";
                             ui->displayingMsg->append("sendFailed");
    });

    // connect
    QObject::connect(se, &FTPSession::connectSucceeded,
                     [&](std::string welcomeMsg) {
                         qDebug() << "welcomeMsg: " << welcomeMsg.c_str();
                         string temp="welcomeMsg: ";
                         temp.append(welcomeMsg);
                         ui->displayingMsg->append(QString::fromStdString(temp));
                     });

    QObject::connect(se, &FTPSession::connectFailedWithMsg, [&](std::string msg) {
        qDebug("connectFailedWithMsg");
        qDebug() << "msg: " << msg.data();
        string temp="connectFailedWithMsg";
        temp.append(msg);
        ui->displayingMsg->append(QString::fromStdString(temp));
    });

    QObject::connect(se, &FTPSession::connectFailed,
                     [&]() { qDebug("connectFailed");
                             ui->displayingMsg->append("connectFailed");});

    QObject::connect(se, &FTPSession::createSocketFailed,
                     [&]() { qDebug() << "createSocketFailed";
                             ui->displayingMsg->append("createSocketFailed");});

    QObject::connect(se, &FTPSession::unableToConnectToServer,
                     [&]() { qDebug() << "unableToConnectToServer";
                             ui->displayingMsg->append("unableToConnectToServer");});

    // login
    QObject::connect(se, &FTPSession::loginSucceeded,
                     [&]() { qDebug("login succeeded");
                             ui->displayingMsg->append("login succeeded");
                             ui->connectButton->setText("断开");
                             isConnected=true;
                             hideFTPFunction(true);
                             se->listWorkingDir();});

    QObject::connect(se, &FTPSession::loginFailedWithMsg, [&](std::string errorMsg) {
        qDebug() << "loginFailedWithMsg";
        qDebug() << "errorMsg: " << errorMsg.data();
        string temp="errorMsg: ";
        temp.append(errorMsg);
        ui->displayingMsg->append("loginFailedWithMsg");
        ui->displayingMsg->append(QString::fromStdString(temp));
    });

    QObject::connect(se, &FTPSession::loginFailed,
                     [&]() { qDebug("loginFailed");
                             ui->displayingMsg->append("loginFailed");});

    // getFilesize
    QObject::connect(se, &FTPSession::getFilesizeSucceeded,
                     [&](long long size) { qDebug() << "file size: " << size;
                                           string temp="file size: ";
                                           temp=temp+to_string(size);
                                           ui->displayingMsg->append(QString::fromStdString(temp));
                                           if(isUploading)uploadFileOffset=size;});

    QObject::connect(se, &FTPSession::getFilesizeFailedWithMsg, [&](std::string msg) {
        qDebug("getFilesizeFailedWithMsg");
        qDebug() << "msg: " << msg.data();
        ui->displayingMsg->append("getFilesizeFailedWithMsg");
        string temp="msg: ";
        temp.append(msg);
        ui->displayingMsg->append(QString::fromStdString(temp));
    });

    // getDir
    QObject::connect(se, &FTPSession::getDirSucceeded,
                     [&](std::string dir) { currentDir=dir;//
                                            qDebug() << "dir: " << dir.data();
                                            string temp="dir: ";
                                            temp.append(dir);
                                            ui->displayingMsg->append(QString::fromStdString(temp));});

    QObject::connect(se, &FTPSession::getDirFailedWithMsg, [&](std::string msg) {
        qDebug("getDirFailedWithMsg");
        qDebug() << "msg: " << msg.data();
        ui->displayingMsg->append("getDirFailedWithMsg");
        string temp="msg: ";
        temp.append(msg);
        ui->displayingMsg->append(QString::fromStdString(temp));
    });

    QObject::connect(se, &FTPSession::getDirFailed,
                     [&]() { qDebug("getDirFailed");
                             ui->displayingMsg->append("getDirFailed");});

    // changeDir
    QObject::connect(se, &FTPSession::changeDirSucceeded,
                     [&]() { qDebug("changeDirSucceeded");
                             previousDir=currentDir;//
                             ui->displayingMsg->append("changeDirSucceeded");
                             se->listWorkingDir();//TODO
                             });

    QObject::connect(se, &FTPSession::changeDirFailedWithMsg, [&](std::string msg) {
        qDebug("changeDirFailedWithMsg");
        qDebug() << "msg: " << msg.data();
        ui->displayingMsg->append("changeDirFailedWithMsg");
        string temp="msg: ";
        temp.append(msg);
        ui->displayingMsg->append(QString::fromStdString(temp));
    });

    QObject::connect(se, &FTPSession::changeDirFailed,
                     [&]() { qDebug("changeDirFailed");
                             ui->displayingMsg->append("changeDirFailed");});

    // setTransferMode
    QObject::connect(
        se, &FTPSession::setTransferModeSucceeded, [&](bool binary) {
            qDebug() << "set transfer mode: " << (binary ? "binary" : "ascii");
            if(binary)ui->displayingMsg->append("set transfer mode: binary");
            else ui->displayingMsg->append("set transfer mode: ascii");
        });

    QObject::connect(se, &FTPSession::setTransferModeFailedWithMsg,
                     [&](std::string msg) {
                         qDebug("setTransferModeFailedWithMsg");
                         qDebug() << "msg: " << msg.data();
                         ui->displayingMsg->append("setTransferModeFailedWithMsg");
                         string temp="msg: ";
                         temp.append(msg);
                         ui->displayingMsg->append(QString::fromStdString(temp));
                     });

    QObject::connect(se, &FTPSession::setTransferModeFailed,
                     [&]() { qDebug("setTransferModeFailed");
                             ui->displayingMsg->append("setTransferModeFailed");});

    // deleteFile
    QObject::connect(se, &FTPSession::deleteFileSucceeded,
                     [&]() { qDebug("deleteFileSucceeded");
                             ui->displayingMsg->append("deleteFileSucceeded");
                             se->listWorkingDir();});

    QObject::connect(se, &FTPSession::deleteFileFailedWithMsg, [&](std::string msg) {
        qDebug("deleteFileFailedWithMsg");
        qDebug() << "msg: " << msg.data();
        ui->displayingMsg->append("deleteFileFailedWithMsg");
        string temp="msg: ";
        temp.append(msg);
        ui->displayingMsg->append(QString::fromStdString(temp));
    });

    QObject::connect(se, &FTPSession::deleteFileFailed,
                     [&]() { qDebug("deleteFileFailed");
                             ui->displayingMsg->append("deleteFileFailed");});

    // makeDir
    QObject::connect(se, &FTPSession::makeDirSucceeded,
                     [&]() { qDebug("makeDirSucceeded");
                             ui->displayingMsg->append("makeDirSucceeded");
                             se->listWorkingDir();});

    QObject::connect(se, &FTPSession::makeDirFailedWithMsg, [&](std::string msg) {
        qDebug("makeDirFailedWithMsg");
        qDebug() << "msg: " << msg.data();
        ui->displayingMsg->append("makeDirFailedWithMsg");
        string temp="msg: ";
        temp.append(msg);
        ui->displayingMsg->append(QString::fromStdString(temp));
    });

    QObject::connect(se, &FTPSession::makeDirFailed,
                     [&]() { qDebug("makeDirFailed");
                             ui->displayingMsg->append("makeDirFailed");});

    // removeDir
    QObject::connect(se, &FTPSession::removeDirSucceeded,
                     [&]() { qDebug("removeDirSucceeded");
                             ui->displayingMsg->append("removeDirSucceeded");
                             se->listWorkingDir();});

    QObject::connect(se, &FTPSession::removeDirFailedWithMsg, [&](std::string msg) {
        qDebug("removeDirFailedWithMsg");
        qDebug() << "msg: " << msg.data();
        ui->displayingMsg->append("removeDirSucceeded");
        string temp="msg: ";
        temp.append(msg);
        ui->displayingMsg->append(QString::fromStdString(temp));
    });

    QObject::connect(se, &FTPSession::removeDirFailed,
                     [&]() { qDebug("removeDirFailed");
                             ui->displayingMsg->append("removeDirFailed");});

    // renameFile
    QObject::connect(se, &FTPSession::renameFileSucceeded,
                     [&]() { qDebug("renameFileSucceeded");
                             ui->displayingMsg->append("renameFileSucceeded");
                             se->listWorkingDir();});

    QObject::connect(se, &FTPSession::renameFileFailedWithMsg, [&](std::string msg) {
        qDebug("renameFileFailedWithMsg");
        qDebug() << "msg: " << msg.data();
        ui->displayingMsg->append("renameFileFailedWithMsg");
        string temp="msg: ";
        temp.append(msg);
        ui->displayingMsg->append(QString::fromStdString(temp));
    });

    QObject::connect(se, &FTPSession::renameFileFailed,
                     [&]() { qDebug("renameFileFailed");
                             ui->displayingMsg->append("renameFileFailed");});

    // listDir
    QObject::connect(se, &FTPSession::listDirSucceeded,
                     [&](std::vector<std::string> dirList) {
                         qDebug("listDirSucceeded");
                         QStringList temp;
                         for (std::string &s : dirList)
                         {   qDebug() << s.data();
                             ui->displayingMsg->append(QString::fromStdString(s));
                             temp.append(QString::fromStdString(s));}
                             qml->setStringList(temp);
                     });

    QObject::connect(se, &FTPSession::listDirFailedWithMsg, [&](std::string msg) {
        qDebug("listDirFailedWithMsg");
        qDebug() << "msg: " << msg.data();
        ui->displayingMsg->append("listDirFailedWithMsg");
        string temp="msg: ";
        temp.append(msg);
        ui->displayingMsg->append(QString::fromStdString(temp));
    });

    QObject::connect(se, &FTPSession::listDirFailed,
                     [&]() { qDebug("listDirFailed");
                             ui->displayingMsg->append("listDirFailed");});

    QObject::connect(se, &FTPSession::setTransferModeSucceeded, [&]() {
            //for now doing nothing here
        });
}

void MainWindow::on_upload_clicked()
{
    QString curPath=QDir::currentPath();
    QString fileName=QFileDialog::getOpenFileName(this, "选择一个文件", curPath, "所有文件(*.*)");
    if(!fileName.isEmpty())
    {
        qDebug()<< fileName.toStdString().data();
        ifstream ifs(fileName.toStdString(), ios_base::in | ios_base::binary);
        this->uploadTask=new UploadFileTask(*se, currentDir, ifs);
        this->connectUploadSignals(uploadTask);
        uploadTask->start();
    }
    else ui->displayingMsg->append("User did not choose a file.");
}


void MainWindow::on_transferModeButton_clicked()
{
    if(isBinary)
    {
        isBinary=false;
        se->setTransferMode(false);
        ui->transferModeButton->setText("ASCII");
    }
    else
    {
        isBinary=true;
        se->setTransferMode(true);
        ui->transferModeButton->setText("BINARY");
    }
}


void MainWindow::on_dir_doubleClicked(const QModelIndex &index)
{
    se->getDir();
    string newDir=currentDir+index.data().toString().toStdString();
    newDir.pop_back();
    newDir.append("/");
    qDebug()<< newDir.data();
    se->changeDir(newDir);

}

void MainWindow::on_newDirButton_clicked()
{
    bool isFinished=false;
    QString newDir=QInputDialog::getText(this, "新目录", "请输入新目录名称", QLineEdit::Normal,
                                         "", &isFinished);
    if(isFinished&&!newDir.isEmpty())
    {
        se->getDir();
        string newDirInput=currentDir+newDir.toStdString();
        newDirInput.append("/");
        se->makeDir(newDirInput);
    }
    else ui->displayingMsg->append("Failed to create a new directory.");
}

void MainWindow::on_dir_clicked(const QModelIndex &index)
{
    qDebug()<< index.data();
    currentItem=index.data().toString().toStdString();
    currentItem.pop_back();
}

void MainWindow::on_returnButton_clicked()
{
    se->changeDir(previousDir);
}

void MainWindow::on_renameButton_clicked()
{
    bool isFinished=false;
    QString newName=QInputDialog::getText(this, "新名称", "请输入新文件名称", QLineEdit::Normal,
                                         "", &isFinished);
    if(isFinished&&!newName.isEmpty())
    {
        se->renameFile(currentItem, newName.toStdString());
    }
    else ui->displayingMsg->append("Failed to create rename a file.");
}

void MainWindow::on_deleteButton_clicked()
{
    QMessageBox::StandardButton result=QMessageBox::question(this, "删除", "确认要删除吗？",
                                                             QMessageBox::Yes|QMessageBox::No,
                                                             QMessageBox::No);
    if(result==QMessageBox::Yes)
    {
        se->deleteFile(currentItem);
//        //TODO:Remove a whole directory
//        string removingDir=currentDir+currentItem;
//        removingDir.append("/");
//        se->removeDir(removingDir);
    }
    else ui->displayingMsg->append("User cancels removing the file.");
}

void MainWindow::on_sizeButton_clicked()
{
    se->getFilesize(currentItem);
}

void MainWindow::on_startButton_clicked()
{
    if(isUploading)
    {
        uploadTask->stop();
        ui->startButton->setText("开始");
    }
    else
    {
        uploadTask->resume(uploadFileOffset);
        ui->startButton->setText("暂停");
    }
}
