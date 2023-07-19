#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QDialog>
#include<QCloseEvent>
#include<QMessageBox>
#include<QMenu>
#include"common.h"
#include"mytablewidgetitem.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainDialog; }
QT_END_NAMESPACE

class CKernel;
class MainDialog : public QDialog
{
    Q_OBJECT
signals:
    void SIG_close();
    void SIG_uploadFile(QString path, QString dir);  //上传什么文件到什么目录
    void SIG_uploadFolder(QString path, QString dir);  //上传什么路径的文件夹到什么目录
    void SIG_downloadFile(int fid, QString dir); //下载文件fid+目录
    void SIG_downloadFolder(int fid, QString dir);
    void SIG_addFolder(QString name, QString dir); //创建文件夹 路径 名字
    void SIG_changeDir( QString dir); //路径跳转
    void SIG_shareFile(QVector<int> fileArray, QString dir);//分享文件列表（文件id）， 目录
    void SIG_getShareByLink(int code, QString dir);//在什么路径下 获取什么分享码的文件
    void SIG_deleteFile(QVector<int> fileArray, QString dir);//删除的文件id，目录
    void SIG_setUploadPause( int timestamp, int isPause); //设置上传暂停的信号 1暂停0继续
    void SIG_setDownloadPause( int timestamp, int isPause); //设置上传暂停的信号 1暂停0继续
public:
    MainDialog(QWidget *parent = nullptr);
    ~MainDialog();

    void closeEvent(QCloseEvent * event);

    friend class CKernel;
private slots:
    void slot_setInfo(QString name);
    void on_pb_file_clicked();

    void on_pb_transmit_clicked();

    void on_pb_share_clicked();

    //点击添加文件
    void on_pb_addFile_clicked();

    //添加文件菜单中，action对应的处理函数
    void slot_addFolder(bool flag);
    void slot_uploadFile(bool flag);
    void slot_uploadFloder(bool flag);
    //右键文件信息菜单中的action处理函数
    void slot_downloadFile(bool flag);
    void slot_shareFile(bool flag);
    void slot_deleteFile(bool flag);
    //断点续传
    void slot_uploadPause(bool flag);
    void slot_uploadResume(bool flag);
    void slot_downloadPause(bool flag);
    void slot_downloadResume(bool flag);


    //选中某一行，把一整行选中
    void on_table_file_cellClicked(int row, int column);
    //右键弹出菜单
    void on_table_file_customContextMenuRequested(const QPoint &pos);
    //上传文件进度条
    void slot_openPath(bool flag);

    void slot_insertUploadFile(FileInfo& info);
    void slot_insertUploadComplete(FileInfo& info);
    void slot_insertShareFileInfo(QString name, int size, QString time, int shareTime); // 添加分享
    void slot_updateUploadFileProgress(int timestamp,int pos);
    void slot_deleteUploadFileByRow(int row);//删除某一行
    //插入文件列表信息
    void slot_insertFileInfo( FileInfo& info);
    //下载文件进度条
    void slot_insertDownloadFile(FileInfo& info);
    void slot_updateDownloadFileProgress(int timestamp,int pos);
    void slot_deleteDownloadFileByRow(int row);//删除某一行
    void slot_insertDownloadComplete(FileInfo& info);
    //删除所有的文件信息
    void slot_deleteAllFileInfo();
    void slot_deleteAllShareInfo();
    void on_table_file_cellDoubleClicked(int row, int column);

    void on_pb_prev_clicked();
    //获取分享文件
    void slot_getShare(bool);

    void on_table_upload_cellClicked(int row, int column);

    void on_table_download_cellClicked(int row, int column);
    //获取文件信息 根据时间戳 断点续传时
    FileInfo& slot_getDownloadFileInfoByTimestamp( int timestamp);
    FileInfo& slot_getUploadFileInfoByTimestamp( int timestamp);
private:

private:
    Ui::MainDialog *ui;

    QMenu m_menuAddFile;
    QMenu m_menuFileInfo;
    QMenu m_menuUpload;
    QMenu m_menuDownload;

};
#endif // MAINDIALOG_H
