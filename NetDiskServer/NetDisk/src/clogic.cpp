#include "clogic.h"

#include <clogic.h>

void CLogic::setNetPackMap()
{
    NetPackMap(_DEF_PACK_REGISTER_RQ)     = &CLogic::RegisterRq;
    NetPackMap(_DEF_PACK_LOGIN_RQ)        = &CLogic::LoginRq;
    NetPackMap(_DEF_PACK_UPLOAD_FILE_RQ)  = &CLogic::UploadFileRq;
    NetPackMap(_DEF_PACK_FILE_CONTENT_RQ) = &CLogic::FileContentRq;
    NetPackMap(_DEF_PACK_GET_FILE_INFO_RQ)= &CLogic::GetFileInfoRq;
    NetPackMap(_DEF_PACK_DOWNLOAD_FILE_RQ)= &CLogic::DownloadFileRq;
    NetPackMap(_DEF_PACK_FILE_HEADER_RS)  = &CLogic::FileHeaderRs;
    NetPackMap(_DEF_PACK_FILE_CONTENT_RS) = &CLogic::FileContentRs;
    NetPackMap(_DEF_PACK_ADD_FOLDER_RQ)   = &CLogic::AddFolderRq;
    NetPackMap(_DEF_PACK_SHARE_FILE_RQ)   = &CLogic::ShareFileRq;
    NetPackMap(_DEF_PACK_MY_SHARE_RQ)     = &CLogic::MyShareRq;
    NetPackMap(_DEF_PACK_GET_SHARE_RQ)    = &CLogic::GetShareRq;
    NetPackMap(_DEF_PACK_DOWNLOAD_FOLDER_RQ) = &CLogic::DownloadFolderRq;
    NetPackMap(_DEF_PACK_DELETE_FILE_RQ)  = &CLogic::DeleteFileRq;
    NetPackMap(_DEF_PACK_CONTINUE_UPLOAD_RQ)   = &CLogic::ContinueUploadRq;
    NetPackMap(_DEF_PACK_CONTINUE_DOWNLOAD_RQ) = &CLogic::ContinueDownloadRq;

}
#define _DEF_COUT_FUNC_    cout << "clientfd:"<< clientfd << " " << __func__ << endl;
#define DEF_PATH "/home/hahaha/0527/NetDisk/"
//注册
void CLogic::RegisterRq(sock_fd clientfd,char* szbuf,int nlen)
{
    //cout << "clientfd:"<< clientfd << __func__ << endl;
    _DEF_COUT_FUNC_
    //拆包
    STRU_REGISTER_RQ * rq = (STRU_REGISTER_RQ*) szbuf;
    STRU_REGISTER_RS rs;
    //根据tel　查询手机号是否存在
    char sqlstr[1000] = "";
    sprintf(sqlstr, "select u_tel from t_user where u_tel = '%s';", rq->tel);
    list<string> lstRes;
    bool res = m_sql->SelectMysql( sqlstr, 1, lstRes );
    if( !res )
        std::cout<< "select fail" << sqlstr << endl;

    if( lstRes.size() != 0 ){
        //存在　返回
        rs.result = tel_is_exist;
    }else{//不存在   查看昵称
        sprintf(sqlstr, "select u_tel from t_user where u_name = '%s';", rq->name);
        lstRes.clear();
        res = m_sql->SelectMysql( sqlstr, 1, lstRes );
        if( !res )
            std::cout<< "select fail" << sqlstr << endl;
        if( lstRes.size() != 0){
            //昵称存在　返回
            rs.result = name_is_exist;
        }else{
            //昵称不存在
            rs.result = register_success;
            //注册成功，写入信息
            sprintf(sqlstr, "insert into t_user(u_tel, u_password, u_name) values ('%s', '%s', '%s');",
                    rq->tel, rq->password, rq->name);
            res = m_sql->UpdataMysql( sqlstr );
            if( !res )
                std::cout<< "updata fail" << sqlstr << endl;

            //网盘　创建该用户的目录 根据id命名　需要先查询id
            sprintf(sqlstr, "select u_id from t_user where u_tel = '%s';", rq->tel);
            lstRes.clear();
            res = m_sql->SelectMysql( sqlstr, 1, lstRes );
            if( !res )
                std::cout<< "select fail" << sqlstr << endl;
            if( lstRes.size() != 0){
                //把id转成string
                int id = stoi( lstRes.front() );
                lstRes.pop_front();
                //默认路径　#define DEF_PATH "/home/hahaha/0527/NetDisk/"
                char pathbuf[_MAX_PATH_SIZE] = "";
                sprintf( pathbuf , "%s%d", DEF_PATH, id );
                //创建路径
                umask(0);  //设置权限掩码
                mkdir( pathbuf, 0777);  //创建目录，权限0777
            }
        }
   }
    SendData( clientfd, (char*)&rs, sizeof(rs) );
}

//登录
void CLogic::LoginRq(sock_fd clientfd ,char* szbuf,int nlen)
{
//    cout << "clientfd:"<< clientfd << __func__ << endl;
    _DEF_COUT_FUNC_
    STRU_LOGIN_RQ* rq = (STRU_LOGIN_RQ*)szbuf;
    STRU_LOGIN_RS rs;
    //根据tel
    char sqlbuf[1000] = "";
    list<string> lstRes;
    sprintf( sqlbuf, "select u_password, u_id, u_name from t_user where u_tel = '%s';", rq->tel );
    bool res = m_sql->SelectMysql( sqlbuf, 3, lstRes );
    if( !res )
        std::cout<< "select fail" << sqlbuf << endl;
    if( lstRes.size() == 0){
        //不存在　返回用户不存在
        rs.result = tel_not_exist;
    }else{
        //存在　password 判断是否一致
        string strPassword = lstRes.front();
        lstRes.pop_front();
        if( strcmp( rq->password, strPassword.c_str() ) != 0){
            //不一致　返回密码错误
            rs.result = password_error;
        }else{
            //一致　返回
            rs.result = login_success;
            int id = atoi(lstRes.front().c_str());
            lstRes.pop_front();
            rs.userid = id;
            string strName = lstRes.front();
            lstRes.pop_front();
            strcpy( rs.name, strName.c_str() );
            //保存用户身份
            //创建用户身份结构map
            //先看在不在　在考虑下线
            UserInfo * info = nullptr;
            if( !m_mapIDToUserInfo.find(id, info) ){
                //查不到　创建
                info = new UserInfo;
            }else{
                //todo: 下线
            }
            info->name = strName;
            info->clientfd = clientfd;
            info->userid = id;
            //写入map
            m_mapIDToUserInfo.insert( id, info );
        }
    }
    SendData( clientfd , (char*)&rs , sizeof rs );
}

void CLogic::UploadFileRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    //拆包
    STRU_UPLOAD_FILE_RQ* rq = (STRU_UPLOAD_FILE_RQ*)szbuf;
    //1. 判断是否秒传　todo
    //判断文件是否已经上传完了
    //根据md5　state=1　查数据库得到id(因为写入用户文件关系用)
    {
        char sqlbuf[1000] = "";
        sprintf( sqlbuf, "select f_id from t_file where f_MD5 = '%s' and f_state = 1;", rq->md5);
        list<string> lstRes;
        bool res = m_sql->SelectMysql(sqlbuf, 1, lstRes);
        if(!res){
            cout<<"select failed"<<endl;   return;
        }
        if(lstRes.size() > 0){//查到了　一上传
            int fileid = stoi(lstRes.front()); lstRes.pop_front();
            //写入用户文件关系　　文件引用计数会自动+1（触发器）
            sprintf( sqlbuf, "insert into t_user_file (u_id, f_id, f_dir, f_name, f_uploadtime) values (%d, %d, '%s', '%s', '%s');",rq->userid, fileid, rq->dir, rq->fileName, rq->time );
            res = m_sql->UpdataMysql( sqlbuf );
            if( !res )
            {
             printf( " update fail: %s\n",sqlbuf);
            }
            //写回复包　返回
            STRU_QUICK_UPLOAD_RS rs;
            rs.result = 1;
            rs.timestamp = rq->timestamp;
            rs.userid = rq->userid;

            SendData(clientfd, (char*)&rs, sizeof(rs));
            return;
        }
    }
    //2. 不能秒传　上传文件
    //2.1 定义文件信息结构体
    FileInfo *info = new FileInfo;
    char strpath[1000] = "";
    sprintf( strpath, "%s%d%s%s", DEF_PATH, rq->userid, rq->dir, rq->md5 ); //_DEF_PATH + userid + dir + name-md5
    info->absolutePath = strpath;   //通过这个写数据库，打开文件，文件名：路径+md5
    //文件名　md5
    info->md5 = rq->md5;
    info->dir = rq->dir;
    info->name = rq->fileName;
    info->time = rq->time;
    info->size = rq->size;
    info->type = rq->type;
    info->Filefd = open( strpath, O_CREAT | O_WRONLY |O_TRUNC , 00777);  //Linux文件IO open
    if( info->Filefd < 0)
    {
        std::cout<< "UploadFileRq: file open fail" << std::endl;
        return;
    }

    //2.2 用map存储文件信息
    int64_t user_time = rq->userid* getNumber() + rq->timestamp;
    m_mapTimestampToFileInfo.insert( user_time, info);

    //3. 查数据库，更新记录
        //3.1 插入文件信息（计数0）
    char sqlbuf[1000] = "";
    sprintf( sqlbuf, "insert into t_file (f_size, f_path, f_MD5, f_count, f_state, f_type) values (%d, '%s', '%s', 0, 0, 'file');", rq->size, strpath, rq->md5);
    bool res = m_sql->UpdataMysql( sqlbuf );
    if( !res )
    {
        printf( " update fail: %s\n",sqlbuf);
    }
        //3.2 查文件id
    sprintf( sqlbuf, " select f_id from t_file where f_path = '%s' and f_MD5 = '%s';", strpath, rq->md5);
    list<string> lstRes;
    res = m_sql->SelectMysql( sqlbuf, 1, lstRes );
    if(!res)
    {
        printf("select fail:%s\n", sqlbuf);
    }
    if( lstRes.size() > 0)
    {
        info->fid = stoi ( lstRes.front() );
    }
    lstRes.clear();
     //3.3 插入用户文件关系，触发器引用计数＋１
     sprintf( sqlbuf, "insert into t_user_file (u_id, f_id, f_dir, f_name, f_uploadtime) values (%d, %d, '%s', '%s', '%s');",rq->userid, info->fid, rq->dir, rq->fileName, rq->time );
     res = m_sql->UpdataMysql( sqlbuf );
     if( !res )
     {
         printf( " update fail: %s\n",sqlbuf);
     }

     STRU_UPLOAD_FILE_RS rs;
     rs.fileid = info->fid;
     rs.result = 1;
     rs.timestamp = rq->timestamp;
     rs.userid = rq->userid;
     //4. 写回复包
     SendData( clientfd, (char*)&rs, sizeof(rs));
}

void CLogic::FileContentRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_

    //1. 拆包
    STRU_FILE_CONTENT_RQ* rq = (STRU_FILE_CONTENT_RQ*)szbuf;
    //2．获取文件信息
    int64_t user_time = rq->userid*getNumber()+rq->timestamp;
    FileInfo* info = nullptr;
    if( !m_mapTimestampToFileInfo.find( user_time, info))
    {
        std::cout<< "FileContentRq: find FileInfo failed" << std::endl;
    }
    //3. 写入
    STRU_FILE_CONTENT_RS rs;
    int len = write(info->Filefd, rq->content, rq->len);
     //3.1 写入失败
    if( len != rq->len )
    {
        //回跳
        rs.result = 0;
        lseek(info->Filefd, -1*len, SEEK_CUR);
    }else{
        //3.2　写入成功
            //更新文件位置
        rs.result = 1;
        info->pos += len;
            //是否到达末尾
        if(info->pos >= info->size ){
               //是　结束 回收map结点 更新数据库文件状态
            // printf("upload file success write to end\n");
            close(info->Filefd);
            m_mapTimestampToFileInfo.erase( user_time );
            delete info;
            info = nullptr;
            //更新数据库文件状态为１ todo
            char sqlbuf[1000] = "";
            sprintf(sqlbuf, "update t_file set f_state = 1 where f_id = %d;", rq->fileid );
            printf("set state = 1 where fid = %d\n", rq->fileid);
            bool res = m_sql->UpdataMysql( sqlbuf );
            if( !res ){
                printf("FileContentRq: UpdataMysql failed: %s\n",sqlbuf);
            }
        }
    }
    //4.返回结果
    rs.fileid = rq->fileid;
    rs.len = rq->len; //成功，向后偏移len　失败，向前len
    rs.timestamp = rq->timestamp;
    rs.userid = rq->userid;

    SendData( clientfd, (char*)&rs, sizeof(rs) );
}

void CLogic::GetFileInfoRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    //1.拆包；
    STRU_GET_FILE_INFO_RQ* rq = (STRU_GET_FILE_INFO_RQ*)szbuf;
    //2. 根据　id dir 在视图查找文件信息
    char sqlBuf[1000] = "";
    sprintf( sqlBuf, "select f_id, f_name, f_size, f_uploadTime, f_type from user_file_info where u_id = %d and f_dir = '%s' and f_state = 1;", rq->userid,rq->dir);
    //std::cout<<rq->userid<<"  "<<rq->dir<<endl;
    list<string> lstRes;
    bool res = m_sql->SelectMysql( sqlBuf, 5 ,lstRes);
    if( !res ){
        printf("GetFileInfoRq: select fail:%s\n",sqlBuf);
        return;
    }
    if( lstRes.size() == 0){
        printf("lstRes.size()==0\n" );
        return;
    }
    int count = lstRes.size() / 5;
    int f_id;
    string f_name;
    int f_size;
    string f_time;
    string f_type;
    int packlen = sizeof(STRU_GET_FILE_INFO_RS)+ count * sizeof(STRU_FILE_INFO);
    STRU_GET_FILE_INFO_RS* rs = (STRU_GET_FILE_INFO_RS*)malloc(packlen);
    rs->init();
    for( int i = 0; i<count; ++i){
        f_id = stoi( lstRes.front());  lstRes.pop_front();
        rs->fileInfo[i].fileid = f_id;

        f_name = lstRes.front(); lstRes.pop_front();
        strcpy( rs->fileInfo[i].name, f_name.c_str());

        f_size = stoi( lstRes.front() ); lstRes.pop_front();
        rs->fileInfo[i].size = f_size;

        f_time = lstRes.front(); lstRes.pop_front();
        strcpy( rs->fileInfo[i].time, f_time.c_str() );

        f_type = lstRes.front(); lstRes.pop_front();
        strcpy( rs->fileInfo[i].filetype, f_type.c_str());
     //   std::cout<< rs->fileInfo[i].fileid<<"  "<<rs->fileInfo[i].name<<"  "<<rs->fileInfo[i].size<<"  "<<rs->fileInfo[i].time<<"  "<<rs->fileInfo[i].filetype<<endl;

    }
    //3. 发送回复
    strcpy( rs->dir, rq->dir);
    rs->count = count;
    SendData(clientfd, (char*)rs, packlen);
    free(rs);
}

void CLogic::DownloadFileRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    STRU_DOWNLOAD_FILE_RQ*rq = (STRU_DOWNLOAD_FILE_RQ*)szbuf;

    //查数据库　查什么？看文件信息结构体里有哪些内容，有哪些是请求包里没有的需要从数据库查
        //f_name f_path f_MD5 f_size
    char sqlBuf[1000] = "";
    sprintf( sqlBuf, "select f_name, f_path, f_MD5, f_size from user_file_info where u_id = %d and f_dir = '%s' and f_id = %d;",rq->userid, rq->dir,rq->fileid);
    list<string> lstRes;
    bool res = m_sql->SelectMysql(sqlBuf, 4, lstRes);
    if( !res ){
        cout<< "DownloadFileRq: select error: "<< sqlBuf << endl;
        return;
    }
    if(lstRes.size() == 0){  //没有返回
        return ;
    }

    string strName = lstRes.front(); lstRes.pop_front();
    string strPath = lstRes.front(); lstRes.pop_front();
    string strMD5 = lstRes.front(); lstRes.pop_front();
    int size = stoi(lstRes.front()); lstRes.pop_front();

    //有　创建文件信息　计算key(userid+timestamp)   写入map

    FileInfo*info = new FileInfo;
    info->absolutePath = strPath;//一定要从数据库查，有可能是秒传要从数据库查真实的路径
    info->dir = rq->dir;
    info->fid = rq->fileid;
    info->md5 = strMD5;
    info->name = strName;
    info->size = size;
    info->type = "file";
    info->Filefd = open(info->absolutePath.c_str(), O_RDONLY);
    if( info->Filefd <= 0)
    {
        cout<< "DownloadFileRq: oprn file error"<< endl;
    }

    int64_t user_time = rq->userid*getNumber()+rq->timestamp;
    m_mapTimestampToFileInfo.insert( user_time, info);
    //发送文件头请求
    STRU_FILE_HEADER_RQ headRq;
    strcpy(headRq.dir , rq->dir);
    strcpy(headRq.md5, info->md5.c_str());
    headRq.size = info->size;
    headRq.fileid = info->fid;
    strcpy( headRq.fileName, info->name.c_str()) ;
    strcpy( headRq.fileType, "file");
    headRq.timestamp = rq->timestamp;
    cout<<"SendData: STRU_FILE_HEADER_RQ"<<endl;
    SendData(clientfd, (char*)&headRq, sizeof(headRq));
}

void CLogic::DownloadFolderRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_;
    //拆包
    STRU_DOWNLOAD_FOLDER_RQ* rq = (STRU_DOWNLOAD_FOLDER_RQ*)szbuf;
    //查数据库　拿到信息
    char sqlbuf[1000] = "";
    sprintf(sqlbuf, "select f_type, f_id, f_name, f_path, f_MD5, f_size, f_dir from user_file_info where u_id = %d and f_dir = '%s' and f_id = %d;",rq->userid, rq->dir, rq->fileid);
    list<string> lstRes;
    bool res = m_sql->SelectMysql(sqlbuf, 7, lstRes);
    if( !res )
    {
        cout<<"select fail"<<sqlbuf<<endl;  return;
    }
    if( lstRes.size() == 0) return;
    string type = lstRes.front(); lstRes.pop_front();
    int timestamp = rq->timestamp;
    //下载文件夹
    DownloadFolder(rq->userid, timestamp, clientfd, lstRes);
}

void CLogic::DownloadFolder(int userid, int& timestamp, sock_fd clientfd, list<string> &lstRes)
{
    _DEF_COUT_FUNC_
    //全查出来　挑着用
    int fileid = stoi(lstRes.front()); lstRes.pop_front();
    string strName = lstRes.front(); lstRes.pop_front();
    string strPath = lstRes.front(); lstRes.pop_front();
    string strMD5 = lstRes.front(); lstRes.pop_front();
    int size = stoi(lstRes.front()); lstRes.pop_front();
    string dir = lstRes.front(); lstRes.pop_front();
    cout<< "fileid :"<<fileid<<endl;
    cout<< "dir:"<<dir<<endl;

    //发送创建文件夹请求
    STRU_FOLDER_HEADER_RQ rq;
    rq.timestamp = ++timestamp;  //时间戳处理
    strcpy(rq.fileName, strName.c_str());
    rq.fileid = fileid;
    strcpy(rq.dir, dir.c_str());

    SendData( clientfd, (char*)&rq, sizeof(rq));
    //拼接出路径

    string newdir = dir + strName +"/";

    //　查询newdir下的所有文件和文件夹信息

    char sqlbuf[1000] = "";
    sprintf(sqlbuf, "select f_type, f_id, f_name, f_path, f_MD5, f_size, f_dir from user_file_info where u_id = %d and f_dir = '%s' ;",userid, newdir.c_str() );
    //cout<< userid<<" "<<newdir<<endl;
    list<string> newlstRes;
    bool res = m_sql->SelectMysql(sqlbuf, 7, newlstRes);
    if( !res )
    {
        cout<<"select fail"<<sqlbuf<<endl;  return;
    }
    while( newlstRes.size() != 0){
        string type = newlstRes.front(); newlstRes.pop_front();
        //cout<<"newlstRes: type = "<<type<<endl;
        if(type == "file"){
            //如果文件　下载文件
            DownloadFile(userid, timestamp, clientfd, newlstRes);
        }else{
            //文件夹　递归
            DownloadFolder(userid, timestamp, clientfd, newlstRes);
        }
    }
}
void CLogic::DownloadFile(int userid, int& timestamp, sock_fd clientfd, list<string> &lstRes)
{
    _DEF_COUT_FUNC_;
    int fileid = stoi(lstRes.front()); lstRes.pop_front();
    string strName = lstRes.front(); lstRes.pop_front();
    string strPath = lstRes.front(); lstRes.pop_front();
    string strMD5 = lstRes.front(); lstRes.pop_front();
    int size = stoi(lstRes.front()); lstRes.pop_front();
    string dir = lstRes.front(); lstRes.pop_front();

    FileInfo*info = new FileInfo;
    info->absolutePath = strPath;//一定要从数据库查，有可能是秒传要从数据库查真实的路径
    info->dir = dir;
    info->fid = fileid;
    info->md5 = strMD5;
    info->name = strName;
    info->size = size;
    info->type = "file";
    info->Filefd = open(info->absolutePath.c_str(), O_RDONLY);
    if( info->Filefd < 0)
    {
        cout<< "DownloadFileRq: oprn file error"<< endl;
    }

    int64_t user_time = userid*getNumber() + (++timestamp);
    m_mapTimestampToFileInfo.insert( user_time, info);
    //发送文件头请求
    STRU_FILE_HEADER_RQ headRq;
    strcpy(headRq.dir , dir.c_str());
    strcpy(headRq.md5, info->md5.c_str());
    headRq.size = info->size;
    headRq.fileid = info->fid;
    strcpy( headRq.fileName, info->name.c_str()) ;
    strcpy( headRq.fileType, "file");
    headRq.timestamp = timestamp;
    cout<<"SendData: STRU_FILE_HEADER_RQ"<<endl;
    SendData(clientfd, (char*)&headRq, sizeof(headRq));
}

void CLogic::FileHeaderRs(sock_fd clientfd, char *szbuf, int nlen)
{
     _DEF_COUT_FUNC_
     //拆包
     STRU_FILE_HEADER_RS * rs = (STRU_FILE_HEADER_RS*)szbuf;

     //查找文件信息
     FileInfo* info = nullptr;
     int64_t user_time = rs->userid* getNumber()+rs->timestamp;
     if(!m_mapTimestampToFileInfo.find(user_time, info)) return;

     //发送文件内容请求
     STRU_FILE_CONTENT_RQ rq;
     rq.fileid = rs->fileid;
     rq.userid = rs->userid;
     rq.timestamp = rs->timestamp;
     //读文件
     rq.len = read( info->Filefd, rq.content, _DEF_BUFFER);
     if(rq.len < 0 )
     {
         perror("read file error");
         return;
     }
     cout<< "send STRU_FILE_CONTENT_RQ"<<endl;
     SendData(clientfd, (char*)&rq, sizeof(rq));
}

void CLogic::FileContentRs(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    //拆包
    STRU_FILE_CONTENT_RS* rs = (STRU_FILE_CONTENT_RS*)szbuf;
    //找到文件信息结构
    int64_t user_time = rs->userid*getNumber()+rs->timestamp;
    FileInfo *info = nullptr;
    if( !m_mapTimestampToFileInfo.find( user_time, info)) return;
    //判断是否成功
    if(rs->result != 1){
        //否　回跳
        lseek(info->Filefd, -1*rs->len, SEEK_CUR);
    }else{
        //成功　偏移位置　pos+=len
        info->pos += rs->len;
        //判断是否结束　是　关闭文件　回收　return；
        if( info->pos >= info->size){
            close(info->Filefd);
            m_mapTimestampToFileInfo.erase(user_time);
            delete info;
            info = nullptr;
            return;
        }
    }
    //读文件
    STRU_FILE_CONTENT_RQ rq;
    rq.len = read(info->Filefd, rq.content, _DEF_BUFFER);
    if( rq.len < 0){
        perror("FileContentRs read error");
        return;
    }
    //写请求　发送请求
    rq.fileid = rs->fileid;
    rq.userid = rs->userid;
    rq.timestamp = rs->timestamp;

    SendData( clientfd, (char*)&rq, sizeof(rq));
}

void CLogic::AddFolderRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    //拆包
    STRU_ADD_FOLDER_RQ * rq = (STRU_ADD_FOLDER_RQ*)szbuf;
    //添加文件信息到数据库
    char pathbuf[1000]="";  //DEF_PATH/id/dir/name
    sprintf(pathbuf, "%s%d%s%s", DEF_PATH, rq->userid, rq->dir, rq->fileName);
    char sqlbuf[1000] = "";
    sprintf(sqlbuf, "insert into t_file (f_size, f_path, f_count, f_MD5, f_state, f_type) values(0, '%s', 0, '?', 1, 'folder');", pathbuf);
    bool res = m_sql->UpdataMysql( sqlbuf );
    if(!res){
        cout<< "update failed:"<<sqlbuf<<endl;
        return;
    }
    //查询　id
    sprintf( sqlbuf, "select f_id from t_file where f_path = '%s'", pathbuf );
    list<string> lstRes;
    res = m_sql->SelectMysql( sqlbuf, 1, lstRes );
    if(!res){
        cout<<" Select fail"<< sqlbuf<<endl;
        return;
    }
    if(lstRes.size() == 0) return;
    int id = stoi( lstRes.front() ); lstRes.pop_front();
    //写入用户文件关系　－－　隐藏　触发器完成
            //u_id, f_id, f_dir, f_name, f_uploadtime
    sprintf(sqlbuf, "insert into t_user_file (u_id, f_id, f_dir, f_name, f_uploadtime) values (%d, %d, '%s', '%s', '%s');",rq->userid,id,rq->dir,rq->fileName,rq->time);
    res = m_sql->UpdataMysql( sqlbuf );
    if(!res){
        cout<< "update failed:"<<sqlbuf<<endl;
        return;
    }
    //创建目录
    umask(0);
    mkdir(pathbuf, 0777);
    //写回复　发送
    STRU_ADD_FOLDER_RS rs;
    rs.result = 1;
    rs.timestamp = rq->timestamp;
    rs.userid = rq->userid;
    SendData( clientfd, (char*)&rs, sizeof(rs));
}

void CLogic::ShareFileRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    //　拆包
    STRU_SHARE_FILE_RQ* rq = (STRU_SHARE_FILE_RQ*)szbuf;
    //生成分享码
    //分享码规则，以数字为例　9位
    int link = 0;
    do{
        //怎么保证9位
        link = random()%9; //随机出1-9
        link *= 100000000;
        link += random() % 100000000;
        //去重　查询分享码是否已存在
        char sqlbuf[1000]= "";
        sprintf( sqlbuf, "select s_link from t_user_file where s_link = %d;", link);
        list<string> lstRes;
        bool res = m_sql->SelectMysql( sqlbuf, 1, lstRes);
        if( !res )
        {
            cout << "select fail:"<<sqlbuf << endl;
            return;
        }
        if( lstRes.size() > 0 )
        {
            link = 0;
        }
    }while(link == 0);

    //遍历所有文件　设置分享码　一次分享的所有文件分享码相同
    int itemCount = rq->itemCount;
    for( int i = 0; i< itemCount; ++i)
    {
        shareItem(rq->userid, rq->fileidArry[i], rq->dir, rq->shareTime, link);
    }
    //写回复包
    STRU_SHARE_FILE_RS rs;
    rs.result = 1;
    SendData( clientfd, (char*)&rs, sizeof(rs) );
}

void CLogic::shareItem(int userid, int fileid, string dir, string time, int link)
{
    char sqlbuf[1000]= "";
    sprintf( sqlbuf, "update t_user_file set s_link = '%d' , s_linkTime = '%s' where u_id = %d and f_id = %d and f_dir = '%s';", link, time.c_str(),
             userid,fileid,dir.c_str() );
   // sprintf( sqlbuf, "update t_user_file set s_link = '%d', s_linkTime = '%s' where u_id = %d and f_id = %d anf f_dir = '%s';", link, time.c_str(), userid, fileid, dir.c_str());
    list<string> lstRes;
    bool res = m_sql->UpdataMysql( sqlbuf );
    if( !res )
    {
        cout << "update fail:"<<sqlbuf << endl;
        return;
    }
}

void CLogic::MyShareRq(sock_fd clientfd, char*szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    //拆包
    STRU_MY_SHARE_RQ* rq = (STRU_MY_SHARE_RQ*)szbuf;
    //查数据库　查询所有分享的文件 f_name, f_size, s_linkTime, s_link user_file_info
    char sqlbuf[1000] = "";
    sprintf( sqlbuf, "select f_name, f_size, s_linkTime, s_link from user_file_info where u_id = %d and s_link is not null and s_linkTime is not null;", rq->userid);
    list<string> lstRes;
    bool res = m_sql->SelectMysql(sqlbuf, 4, lstRes);
    if( !res )
    {
        cout<< "select fail:"<<sqlbuf<<endl;
        return;
    }
    int count = lstRes.size();
    if( (count/4 == 0) || (count%4 != 0))   return;
    //写回复
    int packlen = sizeof( STRU_MY_SHARE_RQ ) + (count/4)*sizeof(STRU_MY_SHARE_FILE);
    STRU_MY_SHARE_RS* rs = (STRU_MY_SHARE_RS*)malloc(packlen);
    rs->init();
    rs->itemCount = count/4;
    for( int i = 0; i<rs->itemCount; ++i)
    {
        string name = lstRes.front(); lstRes.pop_front();
        int size = stoi(lstRes.front()); lstRes.pop_front();
        string linkTime = lstRes.front(); lstRes.pop_front();
        int link = stoi(lstRes.front()); lstRes.pop_front();
        strcpy(rs->items[i].name, name.c_str());
        rs->items[i].size = size;
        strcpy( rs->items[i].time, linkTime.c_str() );
        rs->items[i].shareLink = link;
    }
    SendData( clientfd, (char*)rs, packlen);
    free(rs);

}



void CLogic::GetShareRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    STRU_GET_SHARE_RQ* rq = (STRU_GET_SHARE_RQ*)szbuf;

    //拆包 根据分享码查询文件信息 f_id f_name f_dir(分享人的) f_type u_id（分享人的）
    char sqlbuf[1000] = "";
    sprintf( sqlbuf, "select f_id, f_name, f_dir, f_type, u_id from user_file_info where s_link = %d;", rq->shareLink);
    list<string> lstRes;
    bool res = m_sql->SelectMysql(sqlbuf, 5, lstRes);
    if( !res )
    {
        cout<< "select fail:"<<sqlbuf<<endl; return;
    }
    STRU_GET_SHARE_RS rs;
    if( lstRes.size() == 0)
    {
        rs.result = 0;
        SendData( clientfd, (char*)&rs, sizeof(rs));
    }
    rs.result = 1;
    //遍历列表
    if(lstRes.size() %5 != 0)   return;
    while(lstRes.size() != 0)
    {
        int fileid = stoi(lstRes.front()); lstRes.pop_front();
        string name = lstRes.front(); lstRes.pop_front();
        string fromdir = lstRes.front(); lstRes.pop_front();
        string type = lstRes.front(); lstRes.pop_front();
        int fromuserid = stoi(lstRes.front()); lstRes.pop_front();
        if( type == "file"){
        //是文件
            //插入信息到用户文件关系
            GetShareByFile(rq->userid, fileid, rq->dir, name, rq->time);
        }else{
        //是文件夹
            GetShareByFolder(rq->userid, fileid, rq->dir, name, rq->time, fromuserid, fromdir);
        }
    }
    //发送回复
    strcpy( rs.dir, rq->dir);
    SendData( clientfd, (char*)&rs, sizeof(rs));
}

void CLogic::GetShareByFile(int userid, int fileid, string dir, string name, string time)
{
    //写入　用户文件关系
    char sqlbuf[1000] = "";
    sprintf( sqlbuf, "insert into t_user_file (u_id, f_id, f_dir, f_name, f_uploadtime) values (%d, %d, '%s', '%s', '%s');",userid, fileid, dir.c_str(), name.c_str(), time.c_str() );
    bool res = m_sql->UpdataMysql( sqlbuf );
    if( !res )
    {
        printf( " update fail: %s\n",sqlbuf);
    }

}
void CLogic::GetShareByFolder(int userid, int fileid, string dir, string name, string time, int fromuserid, string fromdir)
{
    //是文件夹
        //把文件夹插入用户文件关系
    GetShareByFile(userid,  fileid,  dir,  name,  time);
        //拼接目录获取人和分享人的都要拼接　/  -->06/
    string newDir = dir + name + "/";
    string newFromDir = fromdir + name + "/";
        //根据新目录　查询分享人文件夹里的文件
    char sqlbuf[1000] = "";
    sprintf( sqlbuf, "select f_id, f_name, f_type from user_file_info where f_dir = '%s' and u_id = %d;", newFromDir.c_str(), fromuserid);
    list<string> lstRes;
    bool res = m_sql->SelectMysql(sqlbuf, 3, lstRes);
    if( !res )
    {
        cout<< "select fail:"<<sqlbuf<<endl; return;
    }
    if(lstRes.size() %3 != 0)   return;
    //查询出文件夹里的文件，遍历列表　递归
    while(lstRes.size() != 0)
    {
        int fileid = stoi(lstRes.front()); lstRes.pop_front();
        string name = lstRes.front(); lstRes.pop_front();
        string type = lstRes.front(); lstRes.pop_front();
        if(type == "file")
        {   //插入用户文件关系
            GetShareByFile(userid,  fileid,  newDir,  name,  time);

        }else
        {
            GetShareByFolder(userid, fileid, newDir, name, time, fromuserid, newFromDir);
        }
    }
}

void CLogic::DeleteFileRq(sock_fd clientfd, char *szbuf, int nlen)
{

    _DEF_COUT_FUNC_
    STRU_DELETE_FILE_RQ *rq = (STRU_DELETE_FILE_RQ*)szbuf;
    //查找文件信息　根据 rq->dir; rq->userid; rq->fileidArray;
    int count = rq->fileCount;
    //删除每个文件
    for( int i = 0; i<count; ++i)
    {
        DeleteOneItem(rq->userid,rq->fileidArray[i],rq->dir);
    }
    //写回复
    STRU_DELETE_FILE_RS rs;
    rs.result = 1;
    strcpy( rs.dir, rq->dir);
    SendData(clientfd, (char*)&rs, sizeof (rs));
}

void CLogic::ContinueDownloadRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    //拆包
    STRU_CONTINUE_DOWNLOAD_RQ * rq = (STRU_CONTINUE_DOWNLOAD_RQ*) szbuf;
    //看是否存在　文件信息
    int64_t user_time = rq->userid*getNumber() + rq->timestamp;
    FileInfo* info = nullptr;
    if( !m_mapTimestampToFileInfo.find(user_time, info) ){
        //没有　创建文件信息结构体　查表获取　添加map
        info = new FileInfo;
        //查数据库　查什么？看文件信息结构体里有哪些内容，有哪些是请求包里没有的需要从数据库查
            //f_name f_path f_MD5 f_size
        char sqlBuf[1000] = "";
        sprintf( sqlBuf, "select f_name, f_path, f_MD5, f_size from user_file_info where u_id = %d and f_dir = '%s' and f_id = %d;",rq->userid, rq->dir,rq->fileid);
        list<string> lstRes;
        bool res = m_sql->SelectMysql(sqlBuf, 4, lstRes);
        if( !res ){
            cout<< "ContinueDownloadRq: select error: "<< sqlBuf << endl;
            return;
        }
        if(lstRes.size() == 0){  //没有返回
            return ;
        }

        string strName = lstRes.front(); lstRes.pop_front();
        string strPath = lstRes.front(); lstRes.pop_front();
        string strMD5 = lstRes.front(); lstRes.pop_front();
        int size = stoi(lstRes.front()); lstRes.pop_front();

        //有　创建文件信息　计算key(userid+timestamp)   写入map

        info->absolutePath = strPath;//一定要从数据库查，有可能是秒传要从数据库查真实的路径
        info->dir = rq->dir;
        info->fid = rq->fileid;
        info->md5 = strMD5;
        info->name = strName;
        info->size = size;
        info->type = "file";
        info->Filefd = open(info->absolutePath.c_str(), O_RDONLY);
        if( info->Filefd <= 0)
        {
            cout<< "DownloadFileRq: oprn file error"<< endl;
            return;
        }

        m_mapTimestampToFileInfo.insert(user_time, info);
    }
    //有　文件指针跳转　pos同步
    lseek(info->Filefd, rq->pos, SEEK_SET);
    info->pos = rq->pos;
    //读文件块　发送文件快请求
    STRU_FILE_CONTENT_RQ contentRq;
    contentRq.len = read( info->Filefd, contentRq.content, _DEF_BUFFER );
    contentRq.fileid = rq->fileid;
    contentRq.userid = rq->userid;
    contentRq.timestamp = rq->timestamp;

    SendData(clientfd, (char*)&contentRq, sizeof(contentRq));

}

void CLogic::ContinueUploadRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    //1.拆包
    STRU_CONTINUE_UPLOAD_RQ*rq = (STRU_CONTINUE_UPLOAD_RQ*)szbuf;
    //2.查找文件信息
    int64_t user_time = rq->userid*getNumber() + rq->timestamp;
    FileInfo* info = nullptr;
    if( !m_mapTimestampToFileInfo.find(user_time, info) ){
    //  2.1 没有 查数据库创建文件信息　写打开文件　文件描述符跳转到末尾
        //没有　创建文件信息结构体　查表获取　添加map
        info = new FileInfo;
        //查数据库　查什么？看文件信息结构体里有哪些内容，有哪些是请求包里没有的需要从数据库查
            //f_name f_path f_MD5 f_size
        char sqlBuf[1000] = "";
        sprintf( sqlBuf, "select f_name, f_path, f_MD5, f_size, f_type from user_file_info where u_id = %d and f_dir = '%s' and f_id = %d;",rq->userid, rq->dir,rq->fileid);
        list<string> lstRes;
        bool res = m_sql->SelectMysql(sqlBuf, 5, lstRes);
        if( !res ){
            cout<< "select error: "<< sqlBuf << endl;return;
        }
        if(lstRes.size() == 0){  return ;}//没有返回

        string strName = lstRes.front(); lstRes.pop_front();
        string strPath = lstRes.front(); lstRes.pop_front();
        string strMD5 = lstRes.front(); lstRes.pop_front();
        int size = stoi(lstRes.front()); lstRes.pop_front();
        string strType = lstRes.front(); lstRes.pop_front();
        //有　创建文件信息　计算key(userid+timestamp)   写入map
        info->absolutePath = strPath;//一定要从数据库查，有可能是秒传要从数据库查真实的路径
        info->dir = rq->dir;
        info->fid = rq->fileid;
        info->md5 = strMD5;
        info->name = strName;
        info->size = size;
        info->type = strType;
        info->Filefd = open(info->absolutePath.c_str(), O_WRONLY);
        if( info->Filefd <= 0)
        {
            cout<< "ContinueUploadRq: oprn file error"<< endl;
            return;
        }
        info->pos = lseek(info->Filefd, 0, SEEK_END);
        m_mapTimestampToFileInfo.insert(user_time, info);
    }
    //  2.2 3 有 回复客户端
    //有　文件指针跳转　pos同步
    //info->pos = lseek(info->Filefd, 0, SEEK_END);

    STRU_CONTINUE_UPLOAD_RS rs;
    rs.pos = info->pos;
    rs.fileid = rq->fileid;
    rs.timestamp = rq->timestamp;

    SendData(clientfd, (char*)&rs, sizeof(rs));
}

void CLogic::DeleteOneItem(int userid, int fileid, string dir)
{
    cout<<"DeleteOneItem"<<endl;
    //删除需要 userid fileid dir 查询type name(文件夹拼接路径)　path（删除文件的路径）
    char sqlbuf[1000] = "";
    sprintf(sqlbuf, "select f_type, f_name, f_path from user_file_info where f_id = %d and u_id = %d and f_dir = '%s';",fileid, userid, dir.c_str());
    list<string> lstRes;
    bool res = m_sql->SelectMysql( sqlbuf, 3, lstRes );
    if( !res )
    {
        cout<< "select fail:"<< sqlbuf <<endl;  return;
    }

    if(lstRes.size() == 0)  return;
    string type = lstRes.front(); lstRes.pop_front();
    string name = lstRes.front(); lstRes.pop_front();
    string path = lstRes.front(); lstRes.pop_front();
    if( type == "file" )
    {
        DeleteFile(userid, fileid, dir, path);
    }else{
        DeleteFolder( userid, fileid, dir, name );
    }
}

void CLogic::DeleteFile(int userid, int fileid, string dir, string path){
    cout<<"DeleteFile"<<endl;
    //删除文件对应的关系
    char sqlbuf[1000] = "";
    sprintf(sqlbuf, "delete from t_user_file where u_id = %d and f_id = %d and f_dir = '%s';", userid, fileid, dir.c_str() );
    bool res = m_sql->UpdataMysql( sqlbuf );
    if( !res )
    {
        cout<< "delete fail:"<< sqlbuf <<endl;  return;
    }
    //再次查询fid看是否能找到结果，如果不能　删除本地文件
    sprintf(sqlbuf, "select from t_user_file where f_id = %d", fileid);
    list<string> lstRes;
    res = m_sql->SelectMysql( sqlbuf, 1, lstRes );
    if( !res )
    {
        cout<< "select fail:"<< sqlbuf << "delete file"<<endl;  return;
    }
    if(lstRes.size() == 0)
    {
        unlink( path.c_str() ); //文件io 删除文件　参数：绝对路径
    }
}

void CLogic::DeleteFolder(int userid, int fileid, string dir, string name)
{
    cout<<"DeleteFolder"<<endl;
    //删除文件夹的用户文件关系
    char sqlbuf[1000] = "";
    sprintf(sqlbuf, "delete from t_user_file where u_id = %d and f_id = %d and f_dir = '%s';", userid, fileid, dir.c_str() );
    bool res = m_sql->UpdataMysql( sqlbuf );
    if( !res )
    {
        cout<< "delete fail:"<< sqlbuf <<endl;  return;
    }
    //拼接新路径
    std::string newDir = dir + name + "/";
    //查表　根据新目录得到f_id f_type name path
    sprintf(sqlbuf, "select f_type, f_id, f_name, f_path from user_file_info where u_id = %d and f_dir = '%s';", userid, newDir.c_str());
    list<string> lstRes;
    res = m_sql->SelectMysql( sqlbuf, 4, lstRes );
    if( !res )
    {
        cout<< "select fail:"<< sqlbuf <<endl;  return;
    }
    while( lstRes.size() != 0){
    //循环
        string type = lstRes.front(); lstRes.pop_front();
        int fileid = stoi(lstRes.front()); lstRes.pop_front();
        string name = lstRes.front(); lstRes.pop_front();
        string path = lstRes.front(); lstRes.pop_front();
        if( type == "file"){
            //删除文件
            DeleteFile(userid, fileid, newDir, path);
        }else{
            //删除文件夹
            DeleteFolder(userid, fileid, newDir, name);
        }
    }
}
