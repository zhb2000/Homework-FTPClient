#include "../include/mainwindow.h"
#include "../include/DownloadFileTask.h"
#include "../include/UploadFileTask.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QFuture>
#include <QInputDialog>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrent>
#include <QtDebug>
#include <iostream>

//#define SET_UI_FONT

using namespace std;
using namespace ftpclient;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      qml(new QStringListModel(this))
{
    ui->setupUi(this);
    hideFTPFunction(false);
    ui->uploadStopButton->setVisible(false);
    ui->uploadPauseResumeButton->setVisible(false);
    ui->uploadProgressBar->setVisible(false);
    ui->downloadStopButton->setVisible(false);
    ui->downloadPauseResumeButton->setVisible(false);
    ui->downloadProgressBar->setVisible(false);

    // initialize listView
    ui->dir->setModel(qml.get());
    ui->uploadListView->setModel(&uploadListModel);
    ui->downloadListView->setModel(&downloadListModel);

    ui->displayingMsg->append(
        "Welcome to the FTP Client designed by ZHB and ZYC.");
    ui->displayingMsg->append("Here display debug messages.");
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::hideFTPFunction(bool value)
{
    ui->download->setVisible(value);
    ui->upload->setVisible(value);
    ui->sizeButton->setVisible(value);
    ui->deleteButton->setVisible(value);
    ui->newDirButton->setVisible(value);
    ui->renameButton->setVisible(value);
    ui->transferModeButton->setVisible(value);
    ui->returnButton->setVisible(value);
    ui->refreshButton->setVisible(value);
    ui->removeButton->setVisible(value);
}

void MainWindow::on_connectButton_clicked()
{
    if (isLogin)
    {
        isLogin = false;
        ui->connectButton->setText("连接");
        hideFTPFunction(false);
        se->quit();
        se.release();
    }
    else
    {
        QString hostname = ui->hostAddressText->toPlainText();
        QString password = ui->passwordText->text();
        QString username = ui->usernameText->toPlainText();
        se = std::unique_ptr<FTPSession>(
            new FTPSession(hostname.toStdString(), username.toStdString(),
                           password.toStdString()));
        this->initConnection(se.get());
        // login
        se->connectAndLogin();
    }
}

void MainWindow::connectUploadSignals(UploadFileTask *task)
{
    QObject::connect(task, &UploadFileTask::uploadStarted, [this]() {
        uploadStatus = TransStatus::RUNNING;

        ui->uploadPauseResumeButton->setText("暂停");
        ui->uploadPauseResumeButton->setVisible(true);
        ui->uploadStopButton->setVisible(true);
        ui->uploadProgressBar->setValue(0);
        ui->uploadProgressBar->setVisible(true);

        ui->displayingMsg->append("uploadStarted");
    });

    QObject::connect(task, &UploadFileTask::uploadSucceeded, [this]() {
        uploadStatus = TransStatus::FREE;
        uploadEndedUISchedule();

        ui->displayingMsg->append("uploadSucceeded");
        QMessageBox::about(this, "上传成功", "上传成功");
        se->listWorkingDir(); //刷新目录
    });

    QObject::connect(
        task, &UploadFileTask::uploadFailedWithMsg,
        [this](std::string errorMsg) {
            uploadStatus = TransStatus::FREE;
            runningUploadTask->stop();
            uploadEndedUISchedule();

            QMessageBox::about(this, "错误",
                               "uploadFailedWithMsg" +
                                   QString::fromStdString(errorMsg));
            ui->displayingMsg->append("uploadFailedWithMsg");
            ui->displayingMsg->append(QString::fromStdString(errorMsg));
        });

    QObject::connect(task, &UploadFileTask::uploadFailed, [this]() {
        uploadStatus = TransStatus::FREE;
        runningUploadTask->stop();
        uploadEndedUISchedule();

        ui->displayingMsg->append("uploadFailed");
        QMessageBox::about(this, "错误", "uploadFailed");
    });

    QObject::connect(task, &UploadFileTask::readFileError, [this]() {
        uploadStatus = TransStatus::FREE;
        runningUploadTask->stop();
        uploadEndedUISchedule();

        ui->displayingMsg->append("readFileError");
        QMessageBox::about(this, "错误", "readFileError");
    });

    QObject::connect(task, &UploadFileTask::uploadPercentage,
                     [this](int percentage) {
                         qDebug() << "percentage: " << percentage << "%";
                         ui->uploadProgressBar->setValue(percentage);
                     });
}

void MainWindow::connectDownloadSignals(DownloadFileTask *task)
{
    QObject::connect(task, &DownloadFileTask::downloadStarted, [this]() {
        downloadStatus = TransStatus::RUNNING;

        ui->downloadPauseResumeButton->setText("暂停");
        ui->downloadPauseResumeButton->setVisible(true);
        ui->downloadStopButton->setVisible(true);
        ui->downloadProgressBar->setVisible(true);
        ui->downloadProgressBar->setValue(0);

        ui->displayingMsg->append("DownloadStarted");
    });

    QObject::connect(task, &DownloadFileTask::downloadSucceed, [this]() {
        downloadStatus = TransStatus::FREE;
        downloadEndedUISchedule();

        ui->displayingMsg->append("DownloadSucceeded");
        QMessageBox::about(this, "下载成功", "下载成功");
    });

    QObject::connect(
        task, &DownloadFileTask::downloadFailedWithMsg,
        [this](std::string errorMsg) {
            downloadStatus = TransStatus::FREE;
            runningDownloadTask->stop();
            downloadEndedUISchedule();

            ui->displayingMsg->append("DownloadFailedWithMsg");
            ui->displayingMsg->append(QString::fromStdString(errorMsg));
            QMessageBox::about(this, "错误",
                               "DownloadFailedWithMsg" +
                                   QString::fromStdString(errorMsg));
        });

    QObject::connect(task, &DownloadFileTask::downloadFailed, [this]() {
        downloadStatus = TransStatus::FREE;
        runningDownloadTask->stop();
        downloadEndedUISchedule();

        ui->displayingMsg->append("DownloadFailed");
        QMessageBox::about(this, "错误", "DownloadFailed");
    });

    QObject::connect(task, &DownloadFileTask::readFileError, [this]() {
        downloadStatus = TransStatus::FREE;
        runningDownloadTask->stop();

        ui->displayingMsg->append("readFileError");
        QMessageBox::about(this, "错误", "readFileError");
    });

    QObject::connect(task, &DownloadFileTask::percentSync, [this](int percent) {
        qDebug() << "percentage: " << percent << "%";
        ui->downloadProgressBar->setValue(percent);
    });
}

void MainWindow::initConnection(FTPSession *se)
{
    QObject::connect(se, &FTPSession::recvFailed, [this]() {
        qDebug() << "recvFailed";
        ui->displayingMsg->append("recvFailed");
        QMessageBox::about(this, "错误", "recvFailed");
    });

    QObject::connect(se, &FTPSession::sendFailed, [this]() {
        qDebug() << "sendFailed";
        ui->displayingMsg->append("sendFailed");
        QMessageBox::about(this, "错误", "sendFailed");
    });

    // connect
    QObject::connect(
        se, &FTPSession::connectSucceeded, [this](std::string welcomeMsg) {
            qDebug() << "welcomeMsg: " << welcomeMsg.c_str();
            string temp = "welcomeMsg: ";
            temp.append(welcomeMsg);
            ui->displayingMsg->append(QString::fromStdString(temp));
        });

    QObject::connect(
        se, &FTPSession::connectFailedWithMsg, [this](std::string msg) {
            qDebug("connectFailedWithMsg");
            qDebug() << "msg: " << msg.data();
            string temp = "connectFailedWithMsg";
            temp.append(msg);
            ui->displayingMsg->append(QString::fromStdString(temp));
            QMessageBox::about(this, "错误", QString::fromStdString(temp));
        });

    QObject::connect(se, &FTPSession::connectFailed, [this]() {
        qDebug("connectFailed");
        ui->displayingMsg->append("connectFailed");
        QMessageBox::about(this, "错误", "connectFailed");
    });

    QObject::connect(se, &FTPSession::createSocketFailed, [this]() {
        qDebug() << "createSocketFailed";
        ui->displayingMsg->append("createSocketFailed");
        QMessageBox::about(this, "错误", "createSocketFailed");
    });

    QObject::connect(se, &FTPSession::unableToConnectToServer, [this]() {
        qDebug() << "unableToConnectToServer";
        ui->displayingMsg->append("unableToConnectToServer");
        QMessageBox::about(this, "错误", "unableToConnectToServer");
    });

    // login
    QObject::connect(se, &FTPSession::loginSucceeded, [this]() {
        qDebug("login succeeded");
        ui->displayingMsg->append("login succeeded");
        ui->connectButton->setText("断开");
        isLogin = true;
        hideFTPFunction(true);
        this->se->listWorkingDir();
    });

    QObject::connect(
        se, &FTPSession::loginFailedWithMsg, [this](std::string errorMsg) {
            qDebug() << "loginFailedWithMsg";
            qDebug() << "errorMsg: " << errorMsg.data();
            string temp = "errorMsg: ";
            temp.append(errorMsg);
            ui->displayingMsg->append("loginFailedWithMsg");
            ui->displayingMsg->append(QString::fromStdString(temp));
            QMessageBox::about(this, "错误",
                               "loginFailedWithMsg: " +
                                   QString::fromStdString(temp));
        });

    QObject::connect(se, &FTPSession::loginFailed, [this]() {
        qDebug("loginFailed");
        ui->displayingMsg->append("loginFailed");
        QMessageBox::about(this, "错误", "loginFailed");
    });

    // getFilesize
    QObject::connect(
        se, &FTPSession::getFilesizeSucceeded, [this](long long size) {
            qDebug() << "file size: " << size;
            string temp = "file size: ";
            temp = temp + to_string(size) + " Byte";
            ui->displayingMsg->append(QString::fromStdString(temp));
            QMessageBox::about(this, "文件大小", QString::fromStdString(temp));
        });

    QObject::connect(
        se, &FTPSession::getFilesizeFailedWithMsg, [this](std::string msg) {
            qDebug("getFilesizeFailedWithMsg");
            qDebug() << "msg: " << msg.data();
            ui->displayingMsg->append("getFilesizeFailedWithMsg");
            string temp = "msg: ";
            temp.append(msg);
            ui->displayingMsg->append(QString::fromStdString(temp));
            QMessageBox::about(this, "错误", QString::fromStdString(temp));
        });

    // getDir
    QObject::connect(se, &FTPSession::getDirSucceeded, [this](std::string dir) {
        currentDir = dir; //
        qDebug() << "dir: " << dir.data();
        string temp = "dir: ";
        temp.append(dir);
        ui->displayingMsg->append(QString::fromStdString(temp));
    });

    QObject::connect(
        se, &FTPSession::getDirFailedWithMsg, [this](std::string msg) {
            qDebug("getDirFailedWithMsg");
            qDebug() << "msg: " << msg.data();
            ui->displayingMsg->append("getDirFailedWithMsg");
            string temp = "msg: ";
            temp.append(msg);
            ui->displayingMsg->append(QString::fromStdString(temp));
            QMessageBox::about(this, "错误", QString::fromStdString(temp));
        });

    QObject::connect(se, &FTPSession::getDirFailed, [this]() {
        qDebug("getDirFailed");
        ui->displayingMsg->append("getDirFailed");
        QMessageBox::about(this, "错误", "getDirFailed");
    });

    // changeDir
    QObject::connect(se, &FTPSession::changeDirSucceeded, [this]() {
        qDebug("changeDirSucceeded");
        ui->displayingMsg->append("changeDirSucceeded");
        this->se->getDir(); //每次切换路径后，更新一下currentDir
        this->se->listWorkingDir();
    });

    QObject::connect(
        se, &FTPSession::changeDirFailedWithMsg, [this](std::string msg) {
            qDebug("changeDirFailedWithMsg");
            qDebug() << "msg: " << msg.data();
            qDebug() << currentDir.data();
            ui->displayingMsg->append("changeDirFailedWithMsg");
            string temp = "msg: ";
            temp.append(msg);
            ui->displayingMsg->append(QString::fromStdString(temp));
            QMessageBox::about(this, "错误", QString::fromStdString(temp));
        });

    QObject::connect(se, &FTPSession::changeDirFailed, [this]() {
        qDebug("changeDirFailed");
        qDebug() << currentDir.data();
        ui->displayingMsg->append("changeDirFailed");
        QMessageBox::about(this, "错误", "changeDirFailed");
    });

    // setTransferMode
    QObject::connect(
        se, &FTPSession::setTransferModeSucceeded, [this](bool binary) {
            qDebug() << "set transfer mode: " << (binary ? "binary" : "ascii");
            if (binary)
                ui->displayingMsg->append("set transfer mode: binary");
            else
                ui->displayingMsg->append("set transfer mode: ascii");
        });

    QObject::connect(
        se, &FTPSession::setTransferModeFailedWithMsg, [this](std::string msg) {
            qDebug("setTransferModeFailedWithMsg");
            qDebug() << "msg: " << msg.data();
            ui->displayingMsg->append("setTransferModeFailedWithMsg");
            string temp = "msg: ";
            temp.append(msg);
            ui->displayingMsg->append(QString::fromStdString(temp));
            QMessageBox::about(this, "错误", QString::fromStdString(temp));
        });

    QObject::connect(se, &FTPSession::setTransferModeFailed, [this]() {
        qDebug() << "setTransferModeFailed";
        ui->displayingMsg->append("setTransferModeFailed");
        QMessageBox::about(this, "错误", "setTransferModeFailed");
    });

    // deleteFile
    QObject::connect(se, &FTPSession::deleteFileSucceeded, [this]() {
        qDebug("deleteFileSucceeded");
        ui->displayingMsg->append("deleteFileSucceeded");
        this->se->listWorkingDir();
    });

    QObject::connect(
        se, &FTPSession::deleteFileFailedWithMsg, [this](std::string msg) {
            qDebug("deleteFileFailedWithMsg");
            qDebug() << "msg: " << msg.data();
            ui->displayingMsg->append("deleteFileFailedWithMsg");
            string temp = "msg: ";
            temp.append(msg);
            ui->displayingMsg->append(QString::fromStdString(temp));
            QMessageBox::about(this, "错误", QString::fromStdString(temp));
        });

    QObject::connect(se, &FTPSession::deleteFileFailed, [this]() {
        qDebug("deleteFileFailed");
        ui->displayingMsg->append("deleteFileFailed");
        QMessageBox::about(this, "错误", "deleteFileFailed");
    });

    // makeDir
    QObject::connect(se, &FTPSession::makeDirSucceeded, [this]() {
        qDebug("makeDirSucceeded");
        ui->displayingMsg->append("makeDirSucceeded");
        this->se->listWorkingDir();
    });

    QObject::connect(
        se, &FTPSession::makeDirFailedWithMsg, [this](std::string msg) {
            qDebug("makeDirFailedWithMsg");
            qDebug() << "msg: " << msg.data();
            ui->displayingMsg->append("makeDirFailedWithMsg");
            string temp = "msg: ";
            temp.append(msg);
            ui->displayingMsg->append(QString::fromStdString(temp));
            QMessageBox::about(this, "错误", QString::fromStdString(temp));
        });

    QObject::connect(se, &FTPSession::makeDirFailed, [this]() {
        qDebug("makeDirFailed");
        ui->displayingMsg->append("makeDirFailed");
        QMessageBox::about(this, "错误", "makeDirFailed");
    });

    // removeDir
    QObject::connect(se, &FTPSession::removeDirSucceeded, [this]() {
        qDebug("removeDirSucceeded");
        ui->displayingMsg->append("removeDirSucceeded");
        this->se->listWorkingDir();
    });

    QObject::connect(
        se, &FTPSession::removeDirFailedWithMsg, [this](std::string msg) {
            qDebug("removeDirFailedWithMsg");
            qDebug() << "msg: " << msg.data();
            ui->displayingMsg->append("removeDirSucceeded");
            string temp = "msg: ";
            temp.append(msg);
            ui->displayingMsg->append(QString::fromStdString(temp));
            QMessageBox::about(this, "错误", QString::fromStdString(temp));
        });

    QObject::connect(se, &FTPSession::removeDirFailed, [this]() {
        qDebug("removeDirFailed");
        ui->displayingMsg->append("removeDirFailed");
        QMessageBox::about(this, "错误", "removeDirFailed");
    });

    // renameFile
    QObject::connect(se, &FTPSession::renameFileSucceeded, [this]() {
        qDebug("renameFileSucceeded");
        ui->displayingMsg->append("renameFileSucceeded");
        this->se->listWorkingDir();
    });

    QObject::connect(
        se, &FTPSession::renameFileFailedWithMsg, [this](std::string msg) {
            qDebug("renameFileFailedWithMsg");
            qDebug() << "msg: " << msg.data();
            ui->displayingMsg->append("renameFileFailedWithMsg");
            string temp = "msg: ";
            temp.append(msg);
            ui->displayingMsg->append(QString::fromStdString(temp));
            QMessageBox::about(this, "错误", QString::fromStdString(temp));
        });

    QObject::connect(se, &FTPSession::renameFileFailed, [this]() {
        qDebug("renameFileFailed");
        ui->displayingMsg->append("renameFileFailed");
        QMessageBox::about(this, "错误", "renameFileFailed");
    });

    // listDir
    QObject::connect(se, &FTPSession::listDirSucceeded,
                     [this](std::vector<std::string> dirList) {
                         qDebug("listDirSucceeded");
                         QStringList temp;
                         for (std::string &s : dirList)
                         {
                             qDebug() << s.data();
                             ui->displayingMsg->append(
                                 QString::fromStdString(s));
                             temp.append(QString::fromStdString(s));
                         }
                         qml->setStringList(temp);
                     });

    QObject::connect(
        se, &FTPSession::listDirFailedWithMsg, [this](std::string msg) {
            qDebug("listDirFailedWithMsg");
            qDebug() << "msg: " << msg.data();
            ui->displayingMsg->append("listDirFailedWithMsg");
            string temp = "msg: ";
            temp.append(msg);
            ui->displayingMsg->append(QString::fromStdString(temp));
            QMessageBox::about(this, "错误", QString::fromStdString(temp));
        });

    QObject::connect(se, &FTPSession::listDirFailed, [this]() {
        qDebug("listDirFailed");
        ui->displayingMsg->append("listDirFailed");
        QMessageBox::about(this, "错误", "listDirFailed");
    });

    QObject::connect(se, &FTPSession::setTransferModeSucceeded, [this]() {
        qDebug("setTransferModeSucceeded");
        ui->displayingMsg->append("etTransferModeSucceeded");
    });
}

void MainWindow::uploadEndedUISchedule()
{
    runningUploadTask.release();
    popUpload();
    ui->uploadProgressBar->setVisible(false);
    ui->uploadPauseResumeButton->setVisible(false);
    ui->uploadStopButton->setVisible(false);
    scheduleUploadQueue();
}

void MainWindow::downloadEndedUISchedule()
{
    runningDownloadTask.release();
    popDownload();
    ui->downloadProgressBar->setVisible(false);
    ui->downloadPauseResumeButton->setVisible(false);
    ui->downloadStopButton->setVisible(false);
    scheduleDownloadQueue();
}

void MainWindow::scheduleUploadQueue()
{
    if (!uploadQueue.empty() && uploadStatus == TransStatus::FREE)
    {
        std::string localFilepath, remoteFilepath;
        std::tie(localFilepath, remoteFilepath) = uploadQueue.front();
        runningUploadTask = std::unique_ptr<UploadFileTask>(
            new UploadFileTask(*se, localFilepath, remoteFilepath));
        connectUploadSignals(runningUploadTask.get());
        runningUploadTask->start();
    }
}

void MainWindow::scheduleDownloadQueue()
{
    if (!downloadQueue.empty() && downloadStatus == TransStatus::FREE)
    {
        std::string localFilepath, remoteFilepath;
        std::tie(localFilepath, remoteFilepath) = downloadQueue.front();
        runningDownloadTask = std::unique_ptr<DownloadFileTask>(
            new DownloadFileTask(*se, localFilepath, remoteFilepath));
        connectDownloadSignals(runningDownloadTask.get());
        runningDownloadTask->start();
    }
}

void MainWindow::pushUpload(const QString &filename,
                            const std::string &localFilepath,
                            const std::string &remoteFilepath)
{
    uploadListModel.insertRow(uploadListModel.rowCount());
    QModelIndex index = uploadListModel.index(uploadListModel.rowCount() - 1);
    uploadListModel.setData(index, filename);
    uploadQueue.push_back({localFilepath, remoteFilepath});
}

void MainWindow::pushDownload(const QString &filename,
                              const std::string &localFilepath,
                              const std::string &remoteFilepath)
{
    downloadListModel.insertRow(downloadListModel.rowCount());
    QModelIndex index =
        downloadListModel.index(downloadListModel.rowCount() - 1);
    downloadListModel.setData(index, filename);
    downloadQueue.push_back({localFilepath, remoteFilepath});
}

void MainWindow::popUpload()
{
    if (!uploadQueue.empty())
    {
        uploadQueue.pop_front();
        uploadListModel.removeRow(0);
    }
}

void MainWindow::popDownload()
{
    if (!downloadQueue.empty())
    {
        downloadQueue.pop_front();
        downloadListModel.removeRow(0);
    }
}

void MainWindow::on_uploadPauseResumeButton_clicked()
{
    if (uploadStatus == TransStatus::RUNNING)
    {
        uploadStatus = TransStatus::PAUSE;
        runningUploadTask->stop();
        ui->uploadPauseResumeButton->setText("恢复");
    }
    else
    {
        runningUploadTask->resume();
    }
}
void MainWindow::on_uploadStopButton_clicked()
{
    if (uploadStatus == TransStatus::RUNNING)
        runningUploadTask->stop();
    uploadStatus = TransStatus::FREE;
    uploadEndedUISchedule();
}

void MainWindow::on_downloadPauseResumeButton_clicked()
{
    if (downloadStatus == TransStatus::RUNNING)
    {
        downloadStatus = TransStatus::PAUSE;
        runningDownloadTask->stop();
        ui->downloadPauseResumeButton->setText("恢复");
    }
    else
    {
        runningDownloadTask->resume();
    }
}
void MainWindow::on_downloadStopButton_clicked()
{
    if (downloadStatus == TransStatus::RUNNING)
        runningDownloadTask->stop();
    downloadStatus = TransStatus::FREE;
    downloadEndedUISchedule();
}

void MainWindow::on_upload_clicked()
{
    QString curPath = QDir::currentPath();
    QString localFilepath = QFileDialog::getOpenFileName(
        this, "选择一个文件", curPath, "所有文件(*.*)");
    if (!localFilepath.isEmpty())
    {
        QString filename = QFileInfo(localFilepath).fileName();
        std::string remoteFilepath = currentDir + "/" + filename.toStdString();
        qDebug() << "upload local filepath:" << localFilepath;
        qDebug() << "upload remote filepath:" << remoteFilepath.data();
        pushUpload(filename, localFilepath.toStdString(), remoteFilepath);
        scheduleUploadQueue();
    }
    else
        ui->displayingMsg->append("User did not choose a file.");
}

void MainWindow::on_download_clicked()
{
    QString curPath = QDir::currentPath();
    QString localFilepath = QFileDialog::getExistingDirectory(
        this, "选择目录", curPath, QFileDialog::ShowDirsOnly);
    if (!localFilepath.isEmpty())
    {
        QString filename = QString::fromStdString(currentItem);
        if (filename.length() >= 2 && filename[0] == '.' && filename[1] == '/')
            filename.remove(0, 2);
        localFilepath += "/" + filename;
        std::string remoteFilepath = currentDir + "/" + currentItem;
        qDebug() << "download local filepath:" << localFilepath;
        qDebug() << "download remote filepath:" << remoteFilepath.data();
        pushDownload(filename, localFilepath.toStdString(), remoteFilepath);
        scheduleDownloadQueue();
    }
    else
        ui->displayingMsg->append("User did not choose a file.");
}

void MainWindow::on_transferModeButton_clicked()
{
    if (isBinary)
    {
        isBinary = false;
        se->setTransferMode(false);
        ui->transferModeButton->setText("ASCII");
    }
    else
    {
        isBinary = true;
        se->setTransferMode(true);
        ui->transferModeButton->setText("BINARY");
    }
}

void MainWindow::on_dir_doubleClicked(const QModelIndex &index)
{
    string newDir = index.data().toString().toStdString();
    newDir.pop_back();
    newDir.append("/");
    qDebug() << newDir.data();
    se->changeDir(newDir);
}

void MainWindow::on_newDirButton_clicked()
{
    bool isFinished = false;
    QString newDir = QInputDialog::getText(this, "新目录", "请输入新目录名称",
                                           QLineEdit::Normal, "", &isFinished);
    if (isFinished && !newDir.isEmpty())
    {
        string newDirInput = newDir.toStdString();
        newDirInput.append("/");
        se->makeDir(newDirInput);
    }
    else
        ui->displayingMsg->append("Failed to create a new directory.");
}

void MainWindow::on_dir_clicked(const QModelIndex &index)
{
    qDebug() << index.data();
    currentItem = index.data().toString().toStdString();
    currentItem.pop_back();
}

void MainWindow::on_returnButton_clicked() { se->changeDir("../"); }

void MainWindow::on_renameButton_clicked()
{
    bool isFinished = false;
    QString newName = QInputDialog::getText(this, "新名称", "请输入新文件名称",
                                            QLineEdit::Normal, "", &isFinished);
    if (isFinished && !newName.isEmpty())
    {
        se->renameFile(currentItem, newName.toStdString());
    }
    else
        ui->displayingMsg->append("Failed to create rename a file.");
}

void MainWindow::on_deleteButton_clicked()
{
    QString text =
        "确认要删除 " + QString::fromStdString(currentItem) + " 吗？";
    QMessageBox::StandardButton result = QMessageBox::question(
        this, "删除", text, QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (result == QMessageBox::Yes)
    {
        se->deleteFile(currentItem);
    }
    else
        ui->displayingMsg->append("User cancels removing the file.");
}

void MainWindow::on_sizeButton_clicked() { se->getFilesize(currentItem); }

void MainWindow::on_refreshButton_clicked() { se->listWorkingDir(); }

void MainWindow::on_emptyButton_clicked() { ui->displayingMsg->clear(); }

void MainWindow::on_removeButton_clicked()
{
    QString text =
        "确认要删除 " + QString::fromStdString(currentItem) + " 吗？";
    QMessageBox::StandardButton result = QMessageBox::question(
        this, "删除", text, QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (result == QMessageBox::Yes)
    {
        se->removeDir(currentItem);
    }
    else
        ui->displayingMsg->append("User cancels removing the directory.");
}
