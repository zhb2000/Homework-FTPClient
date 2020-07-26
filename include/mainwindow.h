#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../include/DownloadFileTask.h"
#include "../include/FTPSession.h"
#include "../include/UploadFileTask.h"
#include <QFuture>
#include <QMainWindow>
#include <QtConcurrent/QtConcurrent>
#include <QtDebug>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    /**
     * @brief 连接服务器信号
     * @author zyc
     * @param FTPSession
     */
    void initConnection(ftpclient::FTPSession *se);

    /**
     * @brief 连接上传信号
     * @author zyc
     * @param UploadFileTask 上传任务
     */
    void connectUploadSignals(ftpclient::UploadFileTask *task);

    /**
     * @brief 连接下载信号
     * @author zyc
     * @param DownloadFileTask 下载任务
     */
    void connectDownloadSignals(ftpclient::DownloadFileTask *task);

private slots:

    //默认的槽函数采用下划线命名的方式，这里不做修改

    void on_connectButton_clicked();

    void on_upload_clicked();

    void on_transferModeButton_clicked();

    void on_dir_doubleClicked(const QModelIndex &index);

    void on_newDirButton_clicked();

    void on_dir_clicked(const QModelIndex &index);

    void on_returnButton_clicked();

    void on_renameButton_clicked();

    void on_deleteButton_clicked();

    void on_sizeButton_clicked();

    void on_startButton_clicked();

    void on_refreshButton_clicked();

    void on_emptyButton_clicked();

    void on_removeButton_clicked();

    void on_download_clicked();

private:
    /**
     * @brief 是否显示服务器操作相关按钮，布尔值为真时显示，假的时候不显示
     * @author zyc
     * @param value 设置是否显示
     */
    void hideFTPFunction(bool value);

    Ui::MainWindow *ui;

    //这里使用QStringListModel来初始化ListView
    std::unique_ptr<QStringListModel> qml;

    bool isLogin = false;
    bool isBinary = true;
    bool isUploading = false;
    bool isDownloading = false;

    // ftpclient::FTPSession *se = nullptr;
    std::unique_ptr<ftpclient::FTPSession> se;

    std::string currentDir = "/";

    //对文件列表进行单击操作时，会记录到currentItem里
    std::string currentItem;

    //记录此时正在上传和下载的个数
    // int uploadCount = 0;
    int downloadCount = 0;
    long long downloadFileOffset = 0;

    std::unique_ptr<ftpclient::UploadFileTask> uploadTask;
    std::unique_ptr<ftpclient::DownloadFileTask> downloadTask;
};
#endif // MAINWINDOW_H

//已知bug：双击文件
//如果双击文件会进入changeDir,然后命令失败
