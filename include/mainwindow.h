#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../include/DownloadFileTask.h"
#include "../include/FTPSession.h"
#include "../include/UploadFileTask.h"
#include <QFuture>
#include <QMainWindow>
#include <QtConcurrent/QtConcurrent>
#include <QtDebug>
#include <deque>
#include <memory>
#include <string>

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
     * @authors zyc, zhb
     * @param UploadFileTask 上传任务
     */
    void connectUploadSignals(ftpclient::UploadFileTask *task);

    /**
     * @brief 连接下载信号
     * @authors zyc, zhb
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

    void on_refreshButton_clicked();

    void on_emptyButton_clicked();

    void on_removeButton_clicked();

    void on_download_clicked();

    void on_downloadPauseResumeButton_clicked();
    void on_downloadStopButton_clicked();
    void on_uploadPauseResumeButton_clicked();
    void on_uploadStopButton_clicked();

private:
    /**
     * @brief 是否显示服务器操作相关按钮，布尔值为真时显示，假的时候不显示
     * @author zyc
     * @param value 设置是否显示
     */
    void hideFTPFunction(bool value);

    /**
     * @brief 若队列非空且此时上传空闲，则调度一个上传任务来执行
     * @author zhb
     */
    void scheduleUploadQueue();

    /**
     * @brief 若队列非空且此时下载空闲，则调度一个下载任务来执行
     * @author zhb
     */
    void scheduleDownloadQueue();

    /**
     * @brief 插入上传任务
     * @author zhb
     * @param filename 文件名
     * @param localFilepath 本地路径
     * @param remoteFilepath 远程路径
     */
    void pushUpload(const QString &filename, const std::string &localFilepath,
                    const std::string &remoteFilepath);

    /**
     * @brief 插入下载任务
     * @author zhb
     * @param filename 文件名
     * @param localFilepath 本地路径
     * @param remoteFilepath 远程路径
     */
    void pushDownload(const QString &filename, const std::string &localFilepath,
                      const std::string &remoteFilepath);

    /**
     * @brief 移除上传任务
     * @author zhb
     */
    void popUpload();

    /**
     * @brief 移除下载任务
     * @author zhb
     */
    void popDownload();

    /**
     * @brief 上传结束：修改UI、出队、调度
     * @author zhb
     */
    void uploadEndedUISchedule();

    /**
     * @brief 下载结束：修改UI、出队、调度
     * @author zhb
     */
    void downloadEndedUISchedule();

    Ui::MainWindow *ui;

    //这里使用QStringListModel来初始化ListView
    std::unique_ptr<QStringListModel> qml;

    bool isLogin = false;
    bool isBinary = true;

    enum class TransStatus
    {
        RUNNING, //进行中
        PAUSE,   //暂停
        FREE     //空闲
    };
    //上传状态
    TransStatus uploadStatus = TransStatus::FREE;
    //下载状态
    TransStatus downloadStatus = TransStatus::FREE;

    std::unique_ptr<ftpclient::FTPSession> se;

    std::string currentDir = "/";

    //对文件列表进行单击操作时，会记录到currentItem里
    std::string currentItem;

    //上传下载队列
    std::unique_ptr<ftpclient::UploadFileTask> runningUploadTask;
    std::unique_ptr<ftpclient::DownloadFileTask> runningDownloadTask;
    std::deque<std::pair<std::string, std::string>> uploadQueue;
    std::deque<std::pair<std::string, std::string>> downloadQueue;
    QStringListModel uploadListModel;
    QStringListModel downloadListModel;
};
#endif // MAINWINDOW_H

//已知bug：双击文件
//如果双击文件会进入changeDir,然后命令失败
