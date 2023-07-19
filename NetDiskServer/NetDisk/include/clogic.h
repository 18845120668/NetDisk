#ifndef CLOGIC_H
#define CLOGIC_H

#include"TCPKernel.h"

class CLogic
{
public:
    CLogic( TcpKernel* pkernel )
    {
        m_pKernel = pkernel;
        m_sql = pkernel->m_sql;
        m_tcp = pkernel->m_tcp;
    }
public:
    //设置协议映射
    void setNetPackMap();
    int getNumber()  //存储文件信息结构体的　userid*number
    {
        return 1000000000;
    }
    /************** 发送数据*********************/
    void SendData( sock_fd clientfd, char*szbuf, int nlen )
    {
        m_pKernel->SendData( clientfd ,szbuf , nlen );
    }
    /************** 网络处理 *********************/
    //注册
    void RegisterRq(sock_fd clientfd, char*szbuf, int nlen);
    //登录
    void LoginRq(sock_fd clientfd, char*szbuf, int nlen);
    //上传文件
    void UploadFileRq(sock_fd clientfd, char*szbuf, int nlen);
    //上传文件块
    void FileContentRq(sock_fd clientfd, char*szbuf, int nlen);
    //获取文件信息请求
    void GetFileInfoRq(sock_fd clientfd, char*szbuf, int nlen);
    //下载文件
    void DownloadFileRq(sock_fd clientfd, char*szbuf, int nlen);
    //下载文件夹
    void DownloadFolderRq(sock_fd clientfd, char*szbuf, int nlen);
    void DownloadFolder(int userid, int& timestamp, sock_fd clientfd, list<string> &lstRes);
    void DownloadFile(int userid, int& timestamp, sock_fd clientfd, list<string> &lstRes);
    //下载文件　处理文件头回复
    void FileHeaderRs(sock_fd clientfd, char*szbuf, int nlen);
    void FileContentRs(sock_fd clientfd, char*szbuf, int nlen);
    // 新建文件夹
    void AddFolderRq(sock_fd clientfd, char*szbuf, int nlen);
    //分享文件请求
    void ShareFileRq(sock_fd clientfd, char*szbuf, int nlen);
    //分享一个文件
    void shareItem( int userid, int fileid, string dir, string time, int link);
    //处理获取我的分享
    void MyShareRq(sock_fd clientfd, char*szbuf, int nlen);
    //处理获取分享请求
    void GetShareRq(sock_fd clientfd, char*szbuf, int nlen);
    void GetShareByFile(int userid, int fileid, string dir, string name, string time);
    void GetShareByFolder(int userid, int fileid, string dir, string name, string time, int fromuserid, string fromdir);
    void DeleteFileRq(sock_fd clientfd, char*szbuf, int nlen);
    //断点续传　下载继续
    void ContinueDownloadRq(sock_fd clientfd, char*szbuf, int nlen);
    void ContinueUploadRq(sock_fd clientfd, char*szbuf, int nlen);
    /*******************************************/


    void DeleteOneItem(int userid, int fileid, string dir);
    void DeleteFile(int userid, int fileid, string dir, string path);
    void DeleteFolder(int userid, int fileid, string dir, string name);
private:
    TcpKernel* m_pKernel;
    CMysql * m_sql;
    Block_Epoll_Net * m_tcp;

    MyMap<int, UserInfo*> m_mapIDToUserInfo;
    //key:userid*1000000000(9个) + timestamp value:fileinfo
    //在上传文件的时候保存文件信息，文件块都写完了回收该结点
    MyMap< int64_t, FileInfo* > m_mapTimestampToFileInfo;

};

#endif // CLOGIC_H
