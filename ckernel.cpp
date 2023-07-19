#include "ckernel.h"
#include<QDebug>
#include<QCoreApplication>
#include<QFileInfo>
#include<QSettings>
#include"TcpClientMediator.h"
#include"TcpServerMediator.h"
#include<QMessageBox>
#include"md5.h"
#include<QDir>

#define NetMap(a)   m_netPackMap[ a - _DEF_PACK_BASE]
void CKernel::setNetPackMap()
{
    memset( m_netPackMap, 0, sizeof(PFUN)*_DEF_PACK_COUNT );
    //key协议头偏移量  value函数指针
    NetMap(_DEF_PACK_LOGIN_RS)        = &CKernel::slot_dealLoginRs;
    NetMap(_DEF_PACK_REGISTER_RS)     = &CKernel::slot_dealRegisterRs;
    NetMap(_DEF_PACK_UPLOAD_FILE_RS)  = &CKernel::slot_dealUploadFileRs;
    NetMap(_DEF_PACK_FILE_CONTENT_RS) = &CKernel::slot_dealFileContentRs;
    NetMap(_DEF_PACK_GET_FILE_INFO_RS)= &CKernel::slot_dealGetFileInfoRs;
    NetMap(_DEF_PACK_FILE_HEADER_RQ)  = &CKernel::slot_dealFileHeaderRq;
    NetMap(_DEF_PACK_FILE_CONTENT_RQ) = &CKernel::slot_dealFileContentRq;
    NetMap(_DEF_PACK_ADD_FOLDER_RS)   = &CKernel::slot_dealAddFolderRs;
    NetMap(_DEF_PACK_QUICK_UPLOAD_RS) = &CKernel::slot_dealQuickUploadRs;
    NetMap(_DEF_PACK_SHARE_FILE_RS)   = &CKernel::slot_dealShareFileRs;
    NetMap(_DEF_PACK_MY_SHARE_RS)     = &CKernel::slot_dealMyshareRs;
    NetMap(_DEF_PACK_GET_SHARE_RS)    = &CKernel::slot_dealGetShareRs;
    NetMap(_DEF_PACK_FOLDER_HEADER_RQ)= &CKernel::slot_dealFolderHeadRq;
    NetMap(_DEF_PACK_DELETE_FILE_RS)  = &CKernel::slot_dealDeleteFileRs;
    NetMap(_DEF_PACK_CONTINUE_UPLOAD_RS) = &CKernel::slot_dealContinueUploadRs;
 }

void CKernel::setSystemPath()
{

    //系统路径： exe同级目录 ./NetDisk
    QString path = QCoreApplication::applicationDirPath() +"/NetDisk";
    //如果没有文件夹 创建
    QDir dir;
    if( !dir.exists(path) )  //只能创建一层
    {
        dir.mkdir( path );
    }
    m_sysPath = path;
}

//生成MD5 static函数 当前文件可用
//生成目标格式 password_1234
#define MD5_KEY "1234"
static std::string getMD5(QString val)
{
    QString str = QString("%1_%2").arg(val).arg(MD5_KEY);
    MD5 md( str.toStdString() );
    qDebug() << str << "md5:" << md.toString().c_str()<< endl;
    return md.toString();
}

#include<QTextCodec>

// QString -> char* gb2312
void Utf8ToGB2312( char* gbbuf , int nlen ,QString& utf8)
{
    //转码的对象
    QTextCodec * gb2312code = QTextCodec::codecForName( "gb2312");
    //QByteArray char 类型数组的封装类 里面有很多关于转码 和 写IO的操作
    QByteArray ba = gb2312code->fromUnicode( utf8 );// Unicode -> 转码对象的字符集

    strcpy_s ( gbbuf , nlen , ba.data() );
}

// char* gb2312 --> QString utf8
QString GB2312ToUtf8( char* gbbuf )
{
    //转码的对象
    QTextCodec * gb2312code = QTextCodec::codecForName( "gb2312");
    //QByteArray char 类型数组的封装类 里面有很多关于转码 和 写IO的操作
    return gb2312code->toUnicode( gbbuf );// 转码对象的字符集 -> Unicode
}
//根据绝对路径获取文件MD5
static std::string getFileMD5( QString path)
{
    //把文件读出来 生成MD5类里生成
    FILE* pFile = nullptr;
    //fopen 如果有中文，支持ANSI编码的可以， path是UTF8的 先转码
    char buf[1000] = "";
    Utf8ToGB2312(buf, 1000, path);
    pFile = fopen( buf , "rb"); //二进制只读打开
    if(!pFile){
        qDebug()<< "getFileMD5 : open file failed";
        return string();  //可以返回一个临时对象
    }
    int len = 0;
    MD5 md;
    do{
        len = fread(buf, 1, 1000, pFile); //缓冲区，一次读多少，读躲到，文件指针，返回值：读成功的次数
        md.update( buf, len); //不断读取拼接文本，更新md5
        QCoreApplication::processEvents( QEventLoop::AllEvents, 100 ); //100hs内取出来事件执行
    }while( len > 0 );
    fclose(pFile);

    qDebug() << "file md5:"<< md.toString().c_str();
    return md.toString().c_str();
}


CKernel::CKernel(QObject *parent) : QObject(parent) , m_id(0), m_curDir("/")
{
    //加载配置文件
    loadIniFile();
    setSystemPath();
    //设置协议映射
    setNetPackMap();
    //退出标志位
    m_quit = false;
    //网络
    m_tcpClient = new TcpClientMediator;
    //是使用信号处理数据的
    connect(m_tcpClient, SIGNAL(SIG_ReadyData(uint,char*,int)),
            this, SLOT(slot_dealClientData(uint,char*,int)) );
    m_tcpClient->OpenNet(m_ip.toStdString().c_str(), m_port.toInt() ); //客户端连接真实地址


    m_loginDialog = new LoginDialog;
    connect( m_loginDialog, SIGNAL(SIG_registerCommit(QString,QString,QString)),
             this, SLOT(slot_registerCommit(QString,QString,QString)));
    connect( m_loginDialog, SIGNAL(SIG_loginCommit(QString,QString)),
             this, SLOT(slot_loginCommit(QString,QString)));

    m_loginDialog->show();

    m_mainDialog = new MainDialog; //在kernel里如果用对象,会销毁 需要new一个堆区的量
    connect( m_mainDialog, SIGNAL(SIG_close()),
             this, SLOT(slot_destory()) );
    //绑定上传文件的信号和槽函数
    connect( m_mainDialog, SIGNAL(SIG_uploadFile(QString,QString)),
             this, SLOT(slot_uploadFile(QString,QString)));
    //更新进度条的信号和槽函数
    connect(this, SIGNAL(SIG_updateUploadFileProgress(int,int)),
            m_mainDialog, SLOT(slot_updateUploadFileProgress(int,int)) );
    connect(this, SIGNAL(SIG_updateDownloadFileProgress(int,int)),
            m_mainDialog, SLOT(slot_updateDownloadFileProgress(int,int)) );

    //下载文件和文件夹的信号
    connect(m_mainDialog, SIGNAL(SIG_downloadFile(int,QString)),
            this, SLOT(slot_downloadFile(int,QString)) );
    connect(m_mainDialog, SIGNAL(SIG_downloadFolder(int,QString)),
            this, SLOT(slot_downloadFolder(int,QString)));
    //创建文件夹
    connect(m_mainDialog, SIGNAL(SIG_addFolder(QString,QString)),
            this, SLOT(slot_addFolder(QString,QString)));

    //路径跳转
    connect( m_mainDialog, SIGNAL(SIG_changeDir(QString)),
             this, SLOT(slot_changeDir(QString)));

    //上传文件夹
    connect( m_mainDialog, SIGNAL(SIG_uploadFolder(QString,QString)),
             this, SLOT(slot_uploadFolder(QString,QString)));

    //分享文件
    connect( m_mainDialog, SIGNAL(SIG_shareFile(QVector<int>,QString)),
             this,SLOT(slot_shareFile(QVector<int>,QString)) );

    //分享码获取文件
    connect( m_mainDialog, SIGNAL(SIG_getShareByLink(int,QString)),
             this, SLOT(slot_getShareByLink(int,QString)) );

    //删除文件
    connect( m_mainDialog, SIGNAL(SIG_deleteFile(QVector<int>,QString)),
             this, SLOT(slot_deleteFile(QVector<int>,QString)));
    //续传
    connect( m_mainDialog, SIGNAL(SIG_setUploadPause(int,int)),
             this, SLOT(slot_setUploadPause(int,int)));
   // connect( m_mainDialog, SIGNAL(SIG_setDownloadPause(int,int)),
      //       this, SLOT(slot_setDownloadPause(int,int)));
    connect(m_mainDialog, SIGNAL(SIG_setDownloadPause(int,int)),
            this, SLOT(slot_setDownloadPause(int,int)));
//    m_mainDialog->show();
    //考虑回收问题：一点×就询问退出，捕捉关闭事件然后回收主窗口 slot_destory

  //  STRU_LOGIN_RQ rq;
  // m_tcpClient->SendData(0, (char*)&rq, sizeof(rq));
}

//配置文件 -->文件位置exe同级目录 -->存在加载，不存在创建并写入默认值
//windows下.ini格式
//[组名]
//key=value 键值对
//例如
//[net]
//ip=10.56.151.172
//port=8004
void CKernel::loadIniFile()
{
    //默认值
    m_ip = "192.168.159.130";
    m_port = "8004";
    //获取exe同级目录 后面没有/的
    QString path = QCoreApplication::applicationDirPath()+"/config.ini";
    //根据目录看是否存在
    QFileInfo info( path );
    if( info.exists() ){  //存在
        QSettings setting(path, QSettings::IniFormat );
        //打开组
        setting.beginGroup( "net" );
        //取键值对
        QVariant strIp = setting.value( "ip", "");  //参数二：默认值 空  返回值：QT中的泛型
        QVariant strPort = setting.value( "port", "");  //QT中的泛型
        if( !strIp.toString().isEmpty() )   m_ip = strIp.toString();
        if( !strPort.toString().isEmpty() ) m_port = strPort.toString();
        //关闭组
        setting.endGroup();
    }else{  //不存在
        QSettings setting(path, QSettings::IniFormat ); //没有会创建
        //打开组
        setting.beginGroup( "net" );
        //设置键值对
        setting.setValue( "ip", m_ip );
        setting.setValue( "port", m_port );
        //关闭组
        setting.endGroup();
    }
    qDebug() << "ip:" << m_ip << " port:"<< m_port << endl;
}

void CKernel::slot_destory()
{
    qDebug() <<__func__;
    m_quit = true;
    m_tcpClient->CloseNet();

    delete m_tcpClient;

    delete m_mainDialog;
    delete m_loginDialog;

}

void CKernel::slot_registerCommit(QString tel, QString password, QString name)
{
    qDebug() <<__func__;
    STRU_REGISTER_RQ rq;
    //名字兼容中文
    std::string strName = name.toStdString();
    strcpy ( rq.name, strName.c_str() );

    strcpy( rq.tel, tel.toStdString().c_str() );

    //MD5
    strcpy(rq.password, getMD5(password).c_str());
    SendData( (char*)&rq, sizeof(rq) );
}

void CKernel::slot_loginCommit(QString tel, QString password)
{
    qDebug() <<__func__;
    STRU_LOGIN_RQ rq;
    //名字兼容中文
    strcpy( rq.tel, tel.toStdString().c_str() );
    //strcpy( rq.password, password.toStdString().c_str() );
    //MD5
    strcpy(rq.password, getMD5(password).c_str());

    SendData( (char*)&rq, sizeof(rq) );
}
#include<QFileInfo>
#include<QDateTime>
//上传文件的槽函数
void CKernel::slot_uploadFile(QString path, QString dir)
{
    qDebug()<<__func__;

    //文件信息存储
    FileInfo info;
    info.absolutePath = path;
    info.dir = dir;
    info.md5 = QString::fromStdString( getFileMD5( path ) );

    QFileInfo qFileinfo(path);
    info.name = qFileinfo.fileName();
    info.size = qFileinfo.size();   //方法二：fopen --> fseek  -->ftell
    //当前时间
    info.time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");  //yyyy-MM-ddd
    info.type = "file";
    //路径需要转码
    char buf[1000] = "";
    Utf8ToGB2312(buf, 1000, path);
    info.pFile = fopen( buf, "rb" ); //C运行时库函数 fopen
    if( !info.pFile )
    {
        qDebug()<< "file open failed" <<endl;
        return;
    }

    //存储到map  map[时间戳] = 文件信息 同一用户可能在不同路径下有一相同文件
    int timestamp = QDateTime::currentDateTime().toString("hhmmsszzz").toInt();
    //bug修复：上传文件夹时，上传太快两个文件的时间戳一致漏上传
    while( m_mapTimestampToFileInfo.count( timestamp )> 0){
        timestamp++;
    }
    info.timestamp = timestamp;
    qDebug()<< "timestamp:" << timestamp << endl;

    m_mapTimestampToFileInfo[ timestamp ] = info;  // 有一个拷贝构造，需不需要自己写一个拷贝构造？
                                        //-->查看对象内容，这里都是int、QString、FILE*文件指针 -->浅拷贝简单的赋值，在这里没有问题
    //发送上传文件请求
    STRU_UPLOAD_FILE_RQ rq;
    //兼容中文
    std::string strDir = dir.toStdString();
    strcpy( rq.dir, strDir.c_str() );
    //兼容中文
    std::string strName = info.name.toStdString();
    strcpy( rq.fileName, strName.c_str() );

    strcpy( rq.fileType, "file" );
    strcpy( rq.md5, info.md5.toStdString().c_str() );
    rq.size = info.size;
    strcpy(rq.time, info.time.toStdString().c_str());
    rq.timestamp = timestamp;
    rq.userid = m_id;
    SendData( (char*)&rq, sizeof(rq) );
}

void CKernel::slot_getCurDirFileList()
{
    //向服务器发送获取当前目录文件列表
    STRU_GET_FILE_INFO_RQ rq;
    rq.userid = m_id;
    std::string strDir = m_curDir.toStdString();
    strcpy( rq.dir, strDir.c_str() );

    SendData( (char*)&rq, sizeof(rq) );
}

void CKernel::slot_downloadFile(int fid, QString dir)
{
    qDebug()<<__func__;
    STRU_DOWNLOAD_FILE_RQ rq;
    //兼容中文
    std::string strDir = dir.toStdString();
    strcpy(rq.dir , strDir.c_str() );
    rq.userid = m_id;
    rq.fileid = fid;
    int timestamp = QDateTime::currentDateTime().toString("hhmmsszzz").toInt() ;

    while( m_mapTimestampToFileInfo.count( timestamp )> 0){
        timestamp++;
    }
    rq.timestamp = timestamp;
    SendData( (char*)&rq, sizeof(rq));
}

void CKernel::slot_downloadFolder(int fid, QString dir)
{
    qDebug()<<__func__;
    STRU_DOWNLOAD_FOLDER_RQ rq;
    std::string strDir = dir.toStdString();
    strcpy(rq.dir, strDir.c_str());
    rq.fileid = fid;
    int timestamp = QDateTime::currentDateTime().toString("hhmmsszzz").toInt();
    if( m_mapTimestampToFileInfo.count(timestamp) > 0){ //去重时间戳 不要和已存在的任务产生冲突
        timestamp++;
    }
    rq.timestamp = timestamp;
    rq.userid = m_id;

    SendData( (char*)&rq, sizeof(rq));
}

void CKernel::slot_addFolder(QString name, QString dir)
{
   qDebug()<<__func__;
   //发包
   STRU_ADD_FOLDER_RQ rq;
   std::string strDir = dir.toStdString();
   strcpy( rq.dir , strDir.c_str());
   std::string strName = name.toStdString();
   strcpy( rq.fileName , strName.c_str());
   string strTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toStdString();
   strcpy(rq.time , strTime.c_str());
   rq.timestamp = QDateTime::currentDateTime().toString("hhmmsszzz").toInt();
   rq.userid = m_id;

   SendData((char*)&rq,sizeof(rq));
}

void CKernel::slot_changeDir(QString dir)
{
    qDebug()<<__func__;
    //更新当前的目录
    m_curDir = dir;
    //刷新列表
    m_mainDialog->slot_deleteAllFileInfo();
    slot_getCurDirFileList();
}

void CKernel::slot_uploadFolder(QString path, QString dir)
{
    qDebug()<<__func__;
    //先获得当前的文件夹，上传addfolder   C:/项目 下有/0314/和 /0527/  1.txt 上传到/05/这个目录
    QFileInfo info(path);
    QDir dr(path);
    qDebug()<< "folder:"<< info.fileName() << "dir" << dir;
    slot_addFolder( info.fileName(), dir );
    //打开文件夹，拿到文件夹下的所有文件列表 文件的路径
    QString newDir = dir+info.fileName()+"/";
    QFileInfoList lst = dr.entryInfoList( ); // 获取所有的文件列表
    //遍历所有文件
    for( int i = 0; i<lst.size(); ++i){
    //如果是.或者..继续，
        QFileInfo file = lst.at(i);  //获取文件信息
        if( file.fileName() == "." )     continue;
        if( file.fileName() == ".." )    continue;
    //如果是文件上传， 路径应该是文件信息的绝对路径 上传到目录 /05/项目
        if( file.isFile()){
            slot_uploadFile( file.absoluteFilePath(), newDir); //file的绝对路径是05
            qDebug()<<"file"<<file.absoluteFilePath()<<"dir:"<<newDir;
        }
    //如果是文件夹，继续这个流程
        if(file.isDir()){
            slot_uploadFolder(file.absoluteFilePath(), newDir);
            qDebug()<<"file"<<file.absoluteFilePath()<<"dir:"<<newDir;
        }
    }
}

void CKernel::slot_shareFile(QVector<int> fileArray, QString dir)
{
    qDebug()<<__func__;
    //发送请求 有柔性数组使用malloc申请空间
    int packlen = sizeof(STRU_SHARE_FILE_RQ) + sizeof(int) * fileArray.size() ;
    STRU_SHARE_FILE_RQ* rq = (STRU_SHARE_FILE_RQ*)malloc( packlen );
    rq->inti();
    rq->itemCount = fileArray.size();
    for( int i = 0; i < rq->itemCount; ++i)
    {
        rq->fileidArry[i] = fileArray[i];
    }
    //路径有中文
    std::string strDir = dir.toStdString();
    strcpy(rq->dir, strDir.c_str());
    rq->userid = m_id;
    QString time =  QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    strcpy( rq->shareTime , time.toStdString().c_str() );

    SendData( (char*)rq, packlen);
    free(rq);


}

//客户端处理数据函数
void CKernel::slot_dealClientData(unsigned int lSendIP, char *buf, int nlen)
{
    //QString str = QString("来自服务端：%1").arg(QString::fromStdString( buf ));
    //QMessageBox::about( NULL, "提示", str); //about是阻塞的模态窗口 kernel不是控件，第一个参数传NULL

    int type = *(int*)buf;
    qDebug()<<__func__;

    if(type >= _DEF_PACK_BASE && type < _DEF_PACK_BASE + _DEF_PACK_COUNT)
    {
        PFUN pf = NetMap(type);
        if(pf){
            (this->*pf)( lSendIP, buf, nlen);  //类成员函数指针需要this调用
        }
    }

    //负责回收buf
    delete[] buf;
}


//处理登录回复
void CKernel::slot_dealLoginRs(unsigned int lSendIP, char *buf, int nlen)
{
    qDebug() <<__func__;
    STRU_LOGIN_RS* rs = (STRU_LOGIN_RS*)buf;

    switch (rs->result) {
    case tel_not_exist:
        QMessageBox::about(m_loginDialog, "提示", "用户不存在,登录失败");
        break;
    case password_error:
        QMessageBox::about(m_loginDialog, "提示", "密码错误,登录失败");
        break;
    case login_success:
        m_loginDialog->hide();
        m_mainDialog->show();
        printf("login success\n");
        //后台
        m_name = rs->name;
        m_id = rs->userid;
        m_mainDialog->slot_setInfo( m_name );
        //获取根目录下的文件列表
        m_curDir = "/";
        slot_getCurDirFileList();
        slot_getMyShare();
        //创建本地数据库 存储上传和下载信息
        InitDatabase(m_id);
        break;
    default:
        break;
    }

}

void CKernel::slot_dealRegisterRs(unsigned int lSendIP, char *buf, int nlen)
{
    qDebug() <<__func__;
    STRU_REGISTER_RS *rs = (STRU_REGISTER_RS*)buf;

    switch (rs->result) {
    case tel_is_exist:
        QMessageBox::about(m_loginDialog, "提示", "用户已存在,注册失败");
        break;
    case name_is_exist:
        QMessageBox::about(m_loginDialog, "提示", "昵称重复,注册失败");
        break;
    case register_success:
         QMessageBox::about(m_loginDialog, "提示", "注册成功");
        break;
    default:
        break;
    }

}

void CKernel::slot_dealUploadFileRs(unsigned int lSendIP, char *buf, int nlen)
{
    qDebug() <<__func__;
    //1. 拆包
    STRU_UPLOAD_FILE_RS* rs = (STRU_UPLOAD_FILE_RS*)buf;
    //2.看结果是否为真
    if( !rs->result )    //为假
    {
        qDebug() << "slot_dealUploadFileRs: rs->result = 0" << endl;
    }else{
        //2.1 为真 获取文件信息 三种方式：count find []下标引用会创建结点
        if (m_mapTimestampToFileInfo.count(rs->timestamp) == 0)
        {
            qDebug()<< "slot_dealUploadFileRs: not found fileinfo"<<endl;
            return;
        }
        //2.2 更新fileid
        FileInfo& info = m_mapTimestampToFileInfo[rs->timestamp];
        info.fileid = rs->fileid;
        //2.3 插入上传信息到上传控件中
        slot_writeUploadTask( info );
        m_mainDialog->slot_insertUploadFile( info );

        //2.4 发送文件块请求
        STRU_FILE_CONTENT_RQ rq;
        rq.len = fread( rq.content, 1, _DEF_BUFFER, info.pFile);
        rq.timestamp = rs->timestamp;
        rq.fileid = rs->fileid;
        rq.userid = m_id;
        SendData( (char*)&rq, sizeof(rq) );
    }
}

void CKernel::slot_dealFileContentRs(unsigned int lSendIP, char *buf, int nlen)
{
    qDebug() <<__func__;
    //1. 拆包
    STRU_FILE_CONTENT_RS* rs = (STRU_FILE_CONTENT_RS*)buf;
    //2.找文件信息结构体
    FileInfo& info = m_mapTimestampToFileInfo[ rs->timestamp ];
    //判断是否暂停
    while(info.isPause){
        //sleep;
        //QT线程类里的sleep
        QThread::msleep( 100 );
        //阻塞行为应该单独写一个线程 槽函数在事件循环里，避免影响事件循环
        //方法二：如果想在主线程中处理 把信号拿出来执行
        QCoreApplication::processEvents( QEventLoop::AllEvents, 100 ); //100hs内取出来事件执行
        if(m_quit)  return; //如果程序退出 直接返回
    }

    //3.看结果是否为真
    if(rs->result == 0)
    {
        //3.1 假 回跳
        fseek( info.pFile, -1*rs->len, SEEK_CUR);
    }else{   //3.2 真 继续读取 pos+len
        info.pos += rs->len;
            //更新上传进度 todo
            Q_EMIT SIG_updateUploadFileProgress( info.timestamp, info.pos );
            //判断文件是否读取结束
        if(info.pos >= info.size){
            //从数据库中删除
            slot_deleteUploadTask( info );
            //是 回收节点 关闭文件 返回
            fclose( info.pFile );
            m_mapTimestampToFileInfo.erase(rs->timestamp);
            //刷新列表
            m_mainDialog->slot_deleteAllFileInfo();
            slot_getCurDirFileList();
            return;
        }
    }
    //4. 发送文件块
    STRU_FILE_CONTENT_RQ rq;
    rq.len = fread(rq.content, 1, _DEF_BUFFER, info.pFile);
    rq.timestamp  =rs->timestamp;
    rq.userid = m_id;
    rq.fileid = rs->fileid;

    SendData( (char*)&rq, sizeof(rq) );
}
//获取文件列表
void CKernel::slot_dealGetFileInfoRs(unsigned int lSendIP, char *buf, int nlen)
{
    qDebug() <<__func__;
    //1. 拆包
    STRU_GET_FILE_INFO_RS* rs = (STRU_GET_FILE_INFO_RS*)buf;

    //判断是否是当前目录，不是就不用更新了
    if( m_curDir != QString::fromStdString(rs->dir)){
        return;
    }
    //先清空
    m_mainDialog->slot_deleteAllFileInfo();
    //2. 设置控件
    int count = rs->count;
    //qDebug()<<"count:"<<count<<endl;
    FileInfo info;
    for( int i = 0; i<count; ++i)
    {
        info.fileid = rs->fileInfo[i].fileid;
        info.type = QString::fromStdString(rs->fileInfo[i].filetype );
        info.name = QString::fromStdString( rs->fileInfo[i].name );
        info.size = rs->fileInfo[i].size;
        info.time = rs->fileInfo[i].time;
        m_mainDialog->slot_insertFileInfo(info);
        //qDebug() << info.fileid<<"  "<<info.type<<"  "<<info.name<<"  "<<info.size<<"  "<<info.time<<endl;
    }
}

void CKernel::slot_dealFileHeaderRq(unsigned int lSendIP, char *buf, int nlen)
{
    qDebug() <<__func__;
    //1. 拆包
    STRU_FILE_HEADER_RQ* rq = (STRU_FILE_HEADER_RQ*)buf;
    //2. 创建文件信息结构体 赋值 打开文件
    FileInfo info;
    // info.absolutePath; //表示文件存在哪 设置一个默认下载路径
    ////m_sysPath不包含\ 如果有多层目录 需要循环创建
     //NetDisk/111/1.txt
    QString tmpDir = QString::fromStdString( rq->dir );
    QStringList dirList = tmpDir.split("/"); //分割函数 根据/分割
    QString pathsum = m_sysPath;
    for( QString & node : dirList )
    {
        if( !node.isEmpty() )
        {
            pathsum += "/";
            pathsum += node;

            QDir dir;
            if( !dir.exists(pathsum) )
            {
                dir.mkdir( pathsum );
            }
        }
    }
    info.name = QString::fromStdString( rq->fileName );
    info.dir = QString::fromStdString( rq->dir ); //目录
    info.absolutePath = m_sysPath + info.dir + info.name;  //m_sysPath不包含\ + 文件夹 + 文件名
    qDebug()<<info.absolutePath<<endl;

    info.fileid = rq->fileid;
    info.md5 = QString::fromStdString( rq->dir );
    info.time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"); //控件上显示的时间 用当前时间就行
    info.timestamp = rq->timestamp;
    info.size = rq->size;
    info.type = "file";
    char pathbuf[1000] = "";
    Utf8ToGB2312(pathbuf, 1000, info.absolutePath);
    info.pFile = fopen(pathbuf, "wb"); //二进制写打开
    if(!info.pFile)
    {
        qDebug()<<"slot_dealFileHeaderRq: open file error";
        return;
    }
    qDebug() << info.fileid<<"  "<<info.type<<"  "<<info.name<<"  "<<info.size<<"  "<<info.time<<endl;
    //3. 保存文件信息结构体
    //保存到数据库
    slot_writeDownloadTask( info );
    m_mapTimestampToFileInfo[rq->timestamp] = info;
    //4. todo 下载文件进度条
    m_mainDialog->slot_insertDownloadFile( info );
    //4. 写回复
    STRU_FILE_HEADER_RS rs;
    rs.fileid = rq->fileid;
    rs.result = 1;
    rs.timestamp = rq->timestamp;
    rs.userid = m_id;
    SendData((char*)&rs, sizeof(rs));
}

void CKernel::slot_dealFileContentRq(unsigned int lSendIP, char *buf, int nlen)
{
    qDebug() <<__func__;
    //1. 拆包
    STRU_FILE_CONTENT_RQ* rq = (STRU_FILE_CONTENT_RQ*)buf;
    //2. 拿到文件信息结构体
    if( m_mapTimestampToFileInfo.count(rq->timestamp) == 0) return;
    FileInfo& info = m_mapTimestampToFileInfo[rq->timestamp];
    //判断是否暂停 暂停挂起
    while(info.isPause){
        //sleep;
        //QT线程类里的sleep
        QThread::msleep( 100 );
        //阻塞行为应该单独写一个线程 槽函数在事件循环里，避免影响事件循环
        //方法二：如果想在主线程中处理 把信号拿出来执行
        QCoreApplication::processEvents( QEventLoop::AllEvents, 100 ); //100hs内取出来事件执行
        if(m_quit)  return; //如果程序退出 直接返回
    }
    //3. 写入
    STRU_FILE_CONTENT_RS rs;
    rs.fileid = rq->fileid;
    rs.len = rq->len; //回跳也要这么长
    rs.timestamp = rq->timestamp;
    rs.userid = rq->userid;
    int len = fwrite(rq->content, 1, rq->len, info.pFile);
        //3.1 写入失败 回跳
    if(len != rq->len){
        rs.result = 0;
        fseek(info.pFile, -1*len, SEEK_CUR);
    }else{
        //3.2 写入成功 pos+=len
        rs.result = 1;
        info.pos += len;
        //todo 更新进度
        Q_EMIT SIG_updateDownloadFileProgress(rq->timestamp, info.pos);
        qDebug()<<"send SIG_updateDownloadFileProgress ";
        if(info.pos >= info.size){
            slot_deleteDownloadTask( info );
            //是否到末尾结束
            fclose(info.pFile);
            //是 关闭文件 回收节点
            m_mapTimestampToFileInfo.erase(rq->timestamp);
        }
    }
    //4. 回复包
    SendData( (char*)&rs, sizeof(rs));
}

void CKernel::slot_dealAddFolderRs(unsigned int lSendIP, char *buf, int nlen)
{
    qDebug() <<__func__;
    //1.拆包
    STRU_ADD_FOLDER_RS* rs = (STRU_ADD_FOLDER_RS*)buf;
    //2.判断是否成功
    if( rs->result != 1) return;
    //3.删除原来的
    m_mainDialog->slot_deleteAllFileInfo();
    //4. 更新文件列表
    slot_getCurDirFileList();
}

void CKernel::slot_dealQuickUploadRs(unsigned int lSendIP, char *buf, int nlen)
{
    qDebug() <<__func__;
    STRU_QUICK_UPLOAD_RS * rs = (STRU_QUICK_UPLOAD_RS*)buf;

    //获取文件信息，关闭文件
    if( m_mapTimestampToFileInfo.count(rs->timestamp) == 0 ) return;
    FileInfo& info = m_mapTimestampToFileInfo[rs->timestamp];
    if(info.pFile)
        fclose(info.pFile);
    //加入已完成信息
    m_mainDialog->slot_insertUploadComplete( info );
    //刷新列表
    if( m_curDir  == info.dir ) // 判断是否是当前目录
    {
        m_mainDialog->slot_deleteAllFileInfo();
        slot_getCurDirFileList();
    }
    //删除节点
    m_mapTimestampToFileInfo.erase( rs->timestamp );
}

void CKernel::slot_dealShareFileRs(unsigned int lSendIP, char *buf, int nlen)
{
    qDebug() <<__func__;
    STRU_SHARE_FILE_RS * rs = (STRU_SHARE_FILE_RS*)buf;
    //判断结果
    if(rs->result != 1) return;
    //刷新 发送获取请求
    slot_getMyShare();
}
void CKernel::slot_getMyShare()
{
    //刷新 发送获取请求
    STRU_MY_SHARE_RQ rq;
    rq.userid = m_id;

    SendData( (char*)&rq, sizeof(rq));
}

void CKernel::slot_getShareByLink(int code, QString dir)
{
    qDebug() <<__func__;
    //发送请求
    STRU_GET_SHARE_RQ rq;
    std::string strDir = dir.toStdString();
    strcpy( rq.dir, strDir.c_str() );
    rq.shareLink = code;
    string time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toStdString();
    strcpy(rq.time, time.c_str());
    rq.userid = m_id;

    SendData( (char*)&rq, sizeof(rq));

}

void CKernel::slot_deleteFile(QVector<int> fileArray, QString dir)
{
    qDebug() <<__func__;
    STRU_DELETE_FILE_RQ *rq;
    int packlen = sizeof(STRU_DELETE_FILE_RQ) + sizeof(int)*fileArray.size();
    rq = (STRU_DELETE_FILE_RQ*)malloc(packlen);
    rq->init();
    rq->fileCount = fileArray.size();
    std::string strDir = dir.toStdString();
    strcpy(rq->dir, strDir.c_str());
    rq->userid = m_id;
    for( int i = 0; i<rq->fileCount; i++)
    {
        rq->fileidArray[i] = fileArray[i];
    }

    SendData((char*)rq, packlen);
    qDebug() << "SendData" <<rq->fileCount <<rq->type<<rq->userid<< rq->dir <<rq->fileidArray[0];
    free(rq);
}

void CKernel::slot_setUploadPause(int timestamp, int isPause)
{
    qDebug() <<__func__;
    //从正在下载变为暂停
    //需要找到文件信息结构体
    if(m_mapTimestampToFileInfo.count(timestamp) > 0){
    //map有 程序未退出
        m_mapTimestampToFileInfo[timestamp].isPause = isPause;
    }else{//map没有 证明程序退出的
        if(isPause == 0){
        //1.获取文件信息 从控件获取
        FileInfo& info = m_mainDialog->slot_getUploadFileInfoByTimestamp( timestamp );  //todo &?
        //2.读打开
        //路径转ASCII码 打开文件 获取文件指针
        char pathBuf[1000] = "";
        Utf8ToGB2312(pathBuf, 1000, info.absolutePath );
        info.pFile = fopen( pathBuf, "rb") ; //二进制读打开 不用二进制大小会出错
        if( !info.pFile ){
            qDebug() << "open file failed:"<<info.absolutePath;
            return;
        }
        info.isPause = 0; //避免开始就停在循环里
        m_mapTimestampToFileInfo[timestamp] = info;
        //3.发上传续传请求
        STRU_CONTINUE_UPLOAD_RQ rq;
        rq.fileid = info.fileid;
        rq.timestamp = timestamp;
        rq.userid = m_id;
        std::string strDir = info.dir.toStdString();
        strcpy(rq.dir, strDir.c_str());
        SendData((char*)&rq, sizeof(rq));
        }
    }
}

void CKernel::slot_setDownloadPause(int timestamp, int isPause)
{
    qDebug() <<__func__;
    if(m_mapTimestampToFileInfo.count(timestamp) > 0){
    //map有 程序未退出
        m_mapTimestampToFileInfo[timestamp].isPause = isPause;
    }else{
        //map没有 证明程序退出的 断点续传
        // 下载的信息 已经存到数据库了 重新登录加载，然后点击开始（继续下载）
        if( isPause == 0 ){
            //断点续传
            //1.创建信息结构体 装入map
            //信息在哪？ 可以从控件里取信息
            FileInfo info = m_mainDialog->slot_getDownloadFileInfoByTimestamp( timestamp );
            //路径转ASCII码 打开文件 获取文件指针
            char pathBuf[1000] = "";
            Utf8ToGB2312(pathBuf, 1000, info.absolutePath );
            info.pFile = fopen( pathBuf, "ab") ;// ab 二进制追加，w会清空
            if( !info.pFile ){
                qDebug() << "open file failed:"<<info.absolutePath;
                return;
            }
            info.isPause = 0; //避免开始就停在循环里
            m_mapTimestampToFileInfo[timestamp] = info;

            //2. 发协议 告诉服务器 文件下载到哪里， 然后服务器跳转到哪里，从哪里开始继续，然后文件块发送
            STRU_CONTINUE_DOWNLOAD_RQ rq;
            rq.timestamp = timestamp;
            rq.pos = info.pos;
            rq.fileid = info.fileid;
            rq.userid = m_id;
            string strdir = info.dir.toStdString();
            strcpy(rq.dir, strdir.c_str());

            SendData((char*)&rq, sizeof(rq));
            //服务器接收 有两种可能 1.文件信息还在(客户端出现异常很快恢复，没有超过预定时间) 关闭文件 2.文件信息不在（超过时间） 重新打开文件
        }
    }
}


void CKernel::slot_dealMyshareRs(unsigned int lSendIP, char *buf, int nlen)
{
    qDebug() <<__func__;
    STRU_MY_SHARE_RS* rs = (STRU_MY_SHARE_RS*)buf;
    //遍历分享文件的信息，添加到控件上
    m_mainDialog->slot_deleteAllShareInfo();
    int count = rs->itemCount;
    for( int i= 0; i<count; ++i)
    {
        m_mainDialog->slot_insertShareFileInfo( rs->items[i].name, rs->items[i].size, rs->items[i].time, rs->items[i].shareLink);

    }
}

void CKernel::slot_dealGetShareRs(unsigned int lSendIP, char *buf, int nlen)
{
    qDebug() <<__func__;
    //拆包
    STRU_GET_SHARE_RS* rs = (STRU_GET_SHARE_RS*)buf;
    //根据结果
    if(rs->result == 0){
        //错误返回提示
        QMessageBox::about( this->m_mainDialog , "提示", "获取分享失败");
    }else{
        //正确刷新列表
        if(QString::fromStdString( rs->dir) == m_curDir){
            slot_getCurDirFileList();  //发请求 获取当前路径的文件列表
        }
    }
}

void CKernel::slot_dealFolderHeadRq(unsigned int lSendIP, char *buf, int nlen)
{
    qDebug() <<__func__;
    //拆包 创建目录
    STRU_FOLDER_HEADER_RQ *rq = (STRU_FOLDER_HEADER_RQ*)buf;
    rq->dir;rq->fileName;
    //创建目录
    //m_sysPath不包含\ 如果有多层目录 需要循环创建
    QString tmpDir = QString::fromStdString( rq->dir );
    QStringList dirList = tmpDir.split("/"); //分割函数 根据/分割
    QString pathsum = m_sysPath;
    for( QString & node : dirList )
    {
        if( !node.isEmpty() ){
            pathsum += "/";
            pathsum += node;

            QDir dir;
            if( !dir.exists(pathsum) ){
                dir.mkdir( pathsum );
            }
        }
    }
    pathsum += "/";
    pathsum += QString::fromStdString(rq->fileName);
    QDir dir;
    if( !dir.exists(pathsum) ){
        dir.mkdir( pathsum );
    }
}

void CKernel::slot_dealDeleteFileRs(unsigned int lSendIP, char *buf, int nlen)
{
    qDebug() <<__func__;
    //拆包 判断结果
    STRU_DELETE_FILE_RS *rs = (STRU_DELETE_FILE_RS*)buf;
    if(rs->result == 1){//刷新列表
        if( QString::fromStdString(rs->dir) == m_curDir)
        {
            m_mainDialog->slot_deleteAllFileInfo();
            slot_getCurDirFileList();
        }
    }

}

void CKernel::slot_dealContinueUploadRs(unsigned int lSendIP, char *buf, int nlen)
{
    qDebug() <<__func__;
    //1，拆包
    STRU_CONTINUE_UPLOAD_RS *rs = (STRU_CONTINUE_UPLOAD_RS*)buf;
    //2.获取文件信息 同步文件信息
    if (m_mapTimestampToFileInfo.count(rs->timestamp) == 0){
        qDebug()<< " not found fileinfo";   return;
    }
    FileInfo& info = m_mapTimestampToFileInfo[rs->timestamp];
    //2.2 更新pos 移动文件指针
    fseek(info.pFile, rs->pos, SEEK_SET);
    info.pos = rs->pos;
    //3.设置进度条？
    m_mainDialog->slot_updateUploadFileProgress(rs->timestamp, info.pos);

    //4. 发文件块请求
    STRU_FILE_CONTENT_RQ rq;
    rq.len = fread( rq.content, 1, _DEF_BUFFER, info.pFile);
    rq.timestamp = rs->timestamp;
    rq.fileid = rs->fileid;
    rq.userid = m_id;
    SendData( (char*)&rq, sizeof(rq) );

}



void CKernel::SendData(char *buf, int nlen)
{
    m_tcpClient->SendData( 0, buf, nlen);
}

void CKernel::InitDatabase(int id)
{

    qDebug()<<__func__;
    m_sql = new CSqlite;
    //首先找到exe的同级目录  创建一个目录  /database/id.db
    QString path = QCoreApplication::applicationDirPath() + "/database/";      //file是exe绝对路径

    QDir dir;
    if( !dir.exists(path)){
        dir.mkdir(path);
    }
    path = path + QString("%1.db").arg(id);
    qDebug()<<path;
    //查看有没有这个文件
    QFileInfo info(path);
    if(! info.exists()){
        //没有 创建表
        QFile file(path);
        if( !file.open( QIODevice::WriteOnly )){
            return;
        }
        file.close();
        //连接
        m_sql->ConnectSql( path );

        //创建表
        QString sqlbuf = "create table t_upload(timestamp int,f_id int,f_name varchar(260),f_dir varchar(260),f_time varchar(60),f_size int,f_md5 varchar(60),f_type varchar(60),f_absolutePath varchar(260));";
        m_sql->UpdateSql( sqlbuf );

        sqlbuf = "create table t_download(timestamp int,f_id int,f_name varchar(260),f_dir varchar(260),f_time varchar(60),f_size int,f_md5 varchar(60),f_type varchar(60),f_absolutePath varchar(260));";
        m_sql->UpdateSql( sqlbuf );
    }

    //有 直接加载
    //连接
    m_sql->ConnectSql( path );

    QList<FileInfo> uploadTaskList;
    QList<FileInfo> downloadTaskList;
    slot_getUploadTask(uploadTaskList);
    slot_getDownloadTask(downloadTaskList);
    //加载上传任务插入到控件中
    for( FileInfo& info : uploadTaskList){
        //如果文件没有了 不能-->继续下一个文件
        QFileInfo fi( info.absolutePath );
        if(!fi.exists()){
            continue;
        }
        //修改任务初始状态
        info.isPause = 1;
        m_mainDialog->slot_insertUploadFile( info );

        //上传续传 控件 看不懂进行多少
        //todo ：获取当前控件位置 同步进度条



    }
    //加载下载任务
    for( FileInfo& info : downloadTaskList){
        //如果文件没有了 不能-->继续下一个文件
        QFileInfo fi( info.absolutePath );
        if(!fi.exists()){
            continue;
        }
        //修改任务初始状态//进行到多少 可以知道
        info.isPause = 1;
        info.pos = fi.size(); //读到当前文件大小
        m_mainDialog->slot_insertDownloadFile( info );  //插入到界面显示
        //获取当前控件位置 同步进度条
        m_mainDialog->slot_updateDownloadFileProgress( info.timestamp,info.pos);
    }

}


void CKernel::slot_writeUploadTask(FileInfo &info)
{
    qDebug()<<__func__;
    QString sqlbuf = QString("insert into t_upload values (%1, %2, '%3', '%4', '%5', %6, '%7', '%8', '%9');").arg(info.timestamp).arg(info.fileid).arg(info.name).arg(info.dir).arg(info.time).arg(info.size).arg(info.md5).arg(info.type).arg(info.absolutePath);
    m_sql->UpdateSql( sqlbuf );
}

void CKernel::slot_writeDownloadTask(FileInfo &info)
{
    qDebug()<<__func__;
    QString sqlbuf = QString("insert into t_download values (%1, %2, '%3', '%4', '%5', %6, '%7', '%8', '%9');").arg(info.timestamp).arg(info.fileid).arg(info.name).arg(info.dir).arg(info.time).arg(info.size).arg(info.md5).arg(info.type).arg(info.absolutePath);
    m_sql->UpdateSql( sqlbuf );
}

void CKernel::slot_deleteUploadTask(FileInfo &info)
{
    qDebug()<<__func__;
    QString sqlbuf = QString("delete from t_upload where timestamp = %1 and f_absolutePath = '%2';").arg(info.timestamp).arg(info.absolutePath) ;
    m_sql->UpdateSql(sqlbuf);
}

void CKernel::slot_deleteDownloadTask(FileInfo &info)
{
    qDebug()<<__func__;
    QString sqlbuf = QString("delete from t_download where timestamp = %1").arg(info.timestamp) ;
    m_sql->UpdateSql(sqlbuf);
}

void CKernel::slot_getUploadTask(QList<FileInfo> &infoList)
{
    qDebug()<<__func__;
    QString sqlbuf = "select * from t_upload;";
    QStringList lst;
    m_sql->SelectSql( sqlbuf, 9, lst );
    while( 0 != lst.size() )
    {
        FileInfo info;
        info.timestamp = QString(lst.front()).toInt(); lst.pop_front();
        info.fileid = QString(lst.front()).toInt(); lst.pop_front();
        info.name = lst.front(); lst.pop_front();
        info.dir = lst.front(); lst.pop_front();
        info.time = lst.front(); lst.pop_front();
        info.size = QString(lst.front()).toInt(); lst.pop_front();
        info.md5 = lst.front(); lst.pop_front();
        info.type = lst.front(); lst.pop_front();
        info.absolutePath = lst.front(); lst.pop_front();

        infoList.push_back( info );
    }
}

void CKernel::slot_getDownloadTask(QList<FileInfo> &infoList)
{
    qDebug()<<__func__;
    QString sqlbuf = "select * from t_download;";
    QStringList lst;
    m_sql->SelectSql( sqlbuf, 9, lst );
    while( 0 != lst.size() )
    {
        FileInfo info;
        info.timestamp = QString(lst.front()).toInt(); lst.pop_front();
        info.fileid = QString(lst.front()).toInt(); lst.pop_front();
        info.name = lst.front(); lst.pop_front();
        info.dir = lst.front(); lst.pop_front();
        info.time = lst.front(); lst.pop_front();
        info.size = QString(lst.front()).toInt(); lst.pop_front();
        info.md5 = lst.front(); lst.pop_front();
        info.type = lst.front(); lst.pop_front();
        info.absolutePath = lst.front(); lst.pop_front();

        infoList.push_back( info );
    }
}


//客户端数据库设计 id.db
//表 正在上传
/*
create table t_upload(
timestamp int,
f_id int,
f_name varchar(260),
f_dir varchar(260),
f_time varchar(60),
f_size int,
f_md5 varchar(60),
f_type varchar(60),
f_absolutePath varchar(260)
);
*/

//表 正在下载
/*
create table t_download(
timestamp int,
f_id int,
f_name varchar(260),
f_dir varchar(260),
f_time varchar(60),
f_size int,
f_md5 varchar(60),
f_type varchar(60),
f_absolutePath varchar(260)
);
*/

