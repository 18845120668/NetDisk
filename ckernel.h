#ifndef CKERNEL_H
#define CKERNEL_H

#include <QObject>
#include<maindialog.h>
#include<logindialog.h>
#include<QThread>
#include"packdef.h"
#include"common.h"
#include"csqlite.h"
#include<QList>

//核心类 设计成单例 只要一个对象
//1.私有化：构造 拷贝构造 析构   2.提供一个静态的公有的获取对象方法
#include<INetMediator.h>

//协议映射表

//类成员函数指针
class CKernel;
typedef void (CKernel::*PFUN)(unsigned int lSendIP , char* buf , int nlen);


class CKernel : public QObject
{
    Q_OBJECT
private:
    explicit CKernel(QObject *parent = nullptr);
    explicit CKernel(const CKernel & kernel);
    ~CKernel(){}

    void loadIniFile();
signals:
    void SIG_updateUploadFileProgress(int timestamp, int pos);
    void SIG_updateDownloadFileProgress(int timestamp, int pos);
public:
    static CKernel* GetInstance(){
        static CKernel kernel; //简单创建在全局区 调用的时候初始化一次 是线程安全的 无法回收
        return &kernel;
    }
private slots:
    ///普通槽函数
    void slot_destory();  //回收主窗口
    void slot_registerCommit(QString tel, QString password, QString name);
    void slot_loginCommit(QString tel, QString password);
    void slot_uploadFile(QString path, QString dir);  //上传文件
    void slot_getCurDirFileList(); //获取当前路径下的文件列表
    void slot_downloadFile(int fid, QString dir); //下载文件
    void slot_downloadFolder(int fid, QString dir);
    void slot_addFolder(QString name, QString dir); //创建文件夹
    void slot_changeDir(QString dir); //路径跳转 更新文件列表
    void slot_uploadFolder(QString path, QString dir); //上传文件夹
    void slot_shareFile(QVector<int> fileArray, QString dir); //分享文件
    void slot_getMyShare();//获取我分享的文件
    void slot_getShareByLink(int code, QString dir); //通过分享码获取文件
    void slot_deleteFile(QVector<int> fileArray, QString dir);//删除的文件id，目录
    //断点续传
    void slot_setUploadPause( int timestamp, int isPause); //设置上传暂停的信号 1暂停0继续
    void slot_setDownloadPause( int timestamp, int isPause); //设置上传暂停的信号 1暂停0继续

    ///网络响应槽函数
    void slot_dealClientData(unsigned int lSendIP , char* buf , int nlen);
    void slot_dealLoginRs(unsigned int lSendIP , char* buf , int nlen);
    void slot_dealRegisterRs(unsigned int lSendIP , char* buf , int nlen);
    void slot_dealUploadFileRs(unsigned int lSendIP , char* buf , int nlen);
    void slot_dealFileContentRs(unsigned int lSendIP , char* buf , int nlen); //上传处理文件内容回复
    void slot_dealGetFileInfoRs(unsigned int lSendIP , char* buf , int nlen);
    void slot_dealFileHeaderRq(unsigned int lSendIP , char* buf , int nlen);
    void slot_dealFileContentRq(unsigned int lSendIP , char* buf , int nlen);//下载处理文件块请求
    void slot_dealAddFolderRs(unsigned int lSendIP , char* buf , int nlen);//处理新建文件夹回复
    void slot_dealQuickUploadRs(unsigned int lSendIP , char* buf , int nlen);
    void slot_dealShareFileRs(unsigned int lSendIP , char* buf , int nlen); //处理分享文件的回复
    void slot_dealMyshareRs(unsigned int lSendIP , char* buf , int nlen);//处理我的分享回复
    void slot_dealGetShareRs(unsigned int lSendIP , char* buf , int nlen);//处理获取别人分享文件的回复
    void slot_dealFolderHeadRq(unsigned int lSendIP , char* buf , int nlen); //文件夹请求，下载文件夹时创建本地文件夹
    void slot_dealDeleteFileRs(unsigned int lSendIP , char* buf , int nlen);
    //处理上传续传
    void slot_dealContinueUploadRs(unsigned int lSendIP , char* buf , int nlen);

    //////////////数据库相关函数/////////////////
private:
    void InitDatabase(int id);
    void slot_writeUploadTask(FileInfo &info);
    void slot_writeDownloadTask(FileInfo &info);
    void slot_deleteUploadTask(FileInfo &info);
    void slot_deleteDownloadTask(FileInfo &info);
    void slot_getUploadTask(QList<FileInfo> &infoList);
    void slot_getDownloadTask(QList<FileInfo> &infoList);

private:
    void setNetPackMap();
    void SendData(char* buf, int nlen);
    void setSystemPath();
private:
    MainDialog* m_mainDialog;
    LoginDialog* m_loginDialog;

    QString m_ip;
    QString m_port;

    QString m_name;
    int m_id;
    QString m_curDir;  //网盘当前的目录
    QString m_sysPath;  //默认下载存储的系统路径 绝对路径 exe同级目录下 NetDisk文件夹下

    INetMediator* m_tcpClient;

    //key：时间戳 (h hmms szzz) --> value：文件信息
    std::map<int, FileInfo > m_mapTimestampToFileInfo; //FileInfo用对象，后面用引用

    PFUN m_netPackMap[_DEF_PACK_COUNT];

    //退出标志位
    bool m_quit;

    CSqlite * m_sql;

};

#endif // CKERNEL_H
