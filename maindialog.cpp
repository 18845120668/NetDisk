#include "maindialog.h"
#include "ui_maindialog.h"
#include<QDebug>
#include<QFileDialog>
#include<QProgressBar>

MainDialog::MainDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MainDialog)
{
    ui->setupUi(this);
    //默认文件分页
    ui->sw_page->setCurrentIndex(0);
    //传输默认分页已完成
    ui->tw_transmit->setCurrentIndex(2);
    //设置标题栏
    this->setWindowTitle("我的网盘");
    //设置最小化和最大化
    this->setWindowFlags(Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint );

    /////////菜单//////////
    //1.定义菜单项 ":/"对应资源文件路径
    QAction * action_addFolder = new QAction( QIcon(":/images/folder.png"),  "新建文件夹");
    QAction * action_uploadFile = new QAction( QIcon(":/images/file.png"),  "上传文件");
    QAction * action_uploadFolder = new QAction( QIcon(":/images/folder.png"),  "上传文件夹");
    //2.添加菜单项
    m_menuAddFile.addAction(action_addFolder);
    m_menuAddFile.addSeparator(); //加入分隔符
    m_menuAddFile.addAction(action_uploadFile);
    m_menuAddFile.addAction(action_uploadFolder);

    connect( action_addFolder, SIGNAL(triggered(bool)),
             this, SLOT(slot_addFolder(bool)) );
    connect( action_uploadFile, SIGNAL(triggered(bool)),
             this, SLOT(slot_uploadFile(bool)) );
    connect( action_uploadFolder, SIGNAL(triggered(bool)),
             this, SLOT(slot_uploadFloder(bool)) );
    ///////////右键文件信息菜单/////////

    QAction * action_downloadFile = new QAction("下载文件");
    QAction * action_share = new QAction("分享文件");
    QAction * action_delete = new QAction("删除文件");
    QAction * action_getshare = new QAction("获取分享");//分享到本地目录

    m_menuFileInfo.addAction( action_addFolder );
    m_menuFileInfo.addSeparator(); //加入分隔符
    m_menuFileInfo.addAction( action_downloadFile );
    m_menuFileInfo.addAction( action_share );
    m_menuFileInfo.addAction( action_delete );
    m_menuFileInfo.addAction( "收藏" ); //没定义action处理动作 先不实现这个功能
    m_menuFileInfo.addSeparator(); //加入分隔符
    m_menuFileInfo.addAction( action_getshare );

    connect( action_downloadFile, SIGNAL(triggered(bool)),
             this, SLOT(slot_downloadFile(bool)) );
    connect( action_share, SIGNAL(triggered(bool)),
             this, SLOT(slot_shareFile(bool)) );
    connect( action_delete, SIGNAL(triggered(bool)),
             this, SLOT(slot_deleteFile(bool)) );
    connect( action_getshare, SIGNAL(triggered(bool)),
             this, SLOT(slot_getShare(bool)) );

    /////////////断点续传菜单////////
    //添加右键显示菜单
    //1. connect(table_download, SIGNAL(), this, SLOT())
    //2. lambda表达式 匿名函数 []捕捉列表 () 函数参数列表 {}函数体 右键弹出菜单
    connect(ui->table_download, &QTableWidget::customContextMenuRequested,
           this, [this]( QPoint ){ this->m_menuDownload.exec( QCursor::pos() ); });
    connect(ui->table_upload, &QTableWidget::customContextMenuRequested,
           this, [this]( QPoint ){ this->m_menuUpload.exec( QCursor::pos() ); });
    //添加菜单项
    QAction *actionUploadPause = new QAction("暂停");
    QAction *actionUploadResume = new QAction("开始");
    m_menuUpload.addAction( actionUploadPause );
    m_menuUpload.addAction( actionUploadResume );
    m_menuUpload.addAction("全部开始");
    m_menuUpload.addAction("全部暂停");

    QAction *actionDownloadPause = new QAction("暂停");
    QAction *actionDownloadResume = new QAction("开始");
    m_menuDownload.addAction(actionDownloadPause);
    m_menuDownload.addAction(actionDownloadResume);
    m_menuDownload.addAction("全部开始");
    m_menuDownload.addAction("全部暂停");
    //处理函数
    connect( actionUploadPause, SIGNAL(triggered(bool)),
             this, SLOT(slot_uploadPause(bool) ) );
    connect( actionUploadResume, SIGNAL(triggered(bool)),
             this, SLOT(slot_uploadResume(bool) ) );
    connect( actionDownloadPause, SIGNAL(triggered(bool)),
             this, SLOT(slot_downloadPause(bool)) );
    connect( actionDownloadResume, SIGNAL(triggered(bool)),
             this, SLOT(slot_downloadResume(bool)) );
}



MainDialog::~MainDialog()
{
    delete ui;
}

void MainDialog::closeEvent(QCloseEvent *event)
{
    //捕捉关闭事件 弹出询问窗口 发送信号让kernel回收

    if(QMessageBox::Yes == QMessageBox::question(this,"退出提示","是否退出?"))
    {
        //关闭
        event->accept();
        Q_EMIT SIG_close();
    }else{
        event->ignore();
    }
}

void MainDialog::slot_setInfo(QString name)
{
    //设置个人信息
    ui->pb_name->setText( name );
}


void MainDialog::on_pb_file_clicked()
{
    ui->sw_page->setCurrentIndex(0);
}


void MainDialog::on_pb_transmit_clicked()
{
    ui->sw_page->setCurrentIndex(1);
}


void MainDialog::on_pb_share_clicked()
{
    ui->sw_page->setCurrentIndex(2);
}

//点击添加文件
void MainDialog::on_pb_addFile_clicked()
{
    //弹出菜单
    m_menuAddFile.exec( QCursor::pos() ); //参数 坐标 使用鼠标坐标
}

//新建文件夹
#include<QInputDialog>
void MainDialog::slot_addFolder(bool flag)
{
    qDebug()<<__func__;
    //弹出输入窗口
    QString name = QInputDialog::getText( this, "新建文件夹", "输入名称");
    QString tmp = name;
    //空白的处理
    if( name.isEmpty() || tmp.remove(" ").isEmpty() || name.length()>100 ){
        QMessageBox::about(this, "提示","名字非法");
        return;
    }
    //非法名称过滤 -->写一个都是非法的文档 正则匹配
    //其他非法 / \ : ? * < > ^ " 非法
    if( name.contains("\\")||name.contains("/")||name.contains("*")||name.contains("^")||name.contains("<")||name.contains(">")||name.contains("\""))
    {
        QMessageBox::about(this, "提示","名字非法");
        return;
    }
    //判断和已有文件夹是否重复 todo

    QString dir = ui->lb_path->text();
    Q_EMIT SIG_addFolder( name, dir);
}

void MainDialog::slot_uploadFile(bool flag)
{
    qDebug()<<__func__;
    //弹窗 选择文件 模态的  getOpenFileNames可以多选
    QString path = QFileDialog::getOpenFileName( this, //按照哪个窗口风格、位置进行初始化
                                  "选择文件", //标题
                                  "./" //默认路径
                ); //返回值 绝对路径。
    if(path.isEmpty())  return;  //判断用户是否取消选择

    //判断目前上传目录是否有一样的文件 todo

    //发信号给核心类处理  传递信息：什么文件， 到什么目录
    QString dir = ui->lb_path->text();
    Q_EMIT SIG_uploadFile( path, dir );
}

void MainDialog::slot_uploadFloder(bool flag)
{
    qDebug()<<__func__;
    //点击 弹出文件选择对话框，选择路径
    QString path = QFileDialog::getExistingDirectory( this, "新建文件夹", "./"); // ./ 只显示目录
    //判断非空
    if( path.isEmpty() ) return;
    //过滤，是否正在上传 todo

    //发信号，上传什么路径的文件夹 到什么 目录
    Q_EMIT SIG_uploadFolder( path, ui->lb_path->text());

}

void MainDialog::slot_downloadFile(bool flag)
{
    qDebug()<<__func__;
    //遍历 获取目录中选中的文件id
    int rows = ui->table_file->rowCount();
    QString dir = ui->lb_path->text();  //获取目录
    for( int i = 0; i<rows; ++i)
    {
        MyTableWidgetItem* item0 =
                (MyTableWidgetItem*)ui->table_file->item(i, 0 );
        //判断选中
        if(item0->checkState() == Qt::Checked ){
            //过滤：判断列表中有没有在下载，避免重复下载 todo


            //获取类型
            if( item0->m_info.type == "file")
            {
                //发信号
                Q_EMIT SIG_downloadFile( item0->m_info.fileid, dir);
            }
            else{
                Q_EMIT SIG_downloadFolder( item0->m_info.fileid, dir);
            }
        }
    }
}

//分享文件
void MainDialog::slot_shareFile(bool flag)
{
    qDebug()<<__func__;
    //申请数组
    QVector<int> array;
    int count = ui->table_file->rowCount();
    //获取所有选中项：遍历所有项目，看是否被选中
    for( int i = 0; i<count; ++i){
        MyTableWidgetItem* item0 = (MyTableWidgetItem*)ui->table_file->item(i , 0);
        if( item0->checkState() == Qt::Checked){//打钩：添加到数组
            array.push_back(item0->m_info.fileid);
        }
    }
    //发送信号
    //传参 用向量
    Q_EMIT SIG_shareFile(array, ui->lb_path->text());
}


void MainDialog::slot_deleteFile(bool flag)
{
    qDebug()<<__func__;
    //遍历 获取选中的所有文件
    int row = ui->table_file->rowCount();
    QVector<int> idArr;
    for(int i = 0; i<row; ++i){
        MyTableWidgetItem* item = (MyTableWidgetItem*)ui->table_file->item(i, 0);
        if(item->checkState() == Qt::Checked)
        {
            idArr.push_back(item->m_info.fileid);
        }
    }

    SIG_deleteFile(idArr, ui->lb_path->text());

}

void MainDialog::slot_uploadPause(bool flag)
{
    qDebug()<< "上传暂停";
    //遍历列表 是否打钩
    int count = ui->table_upload->rowCount();
    for( int i = 0; i<count; ++i){
        MyTableWidgetItem* item0 = (MyTableWidgetItem*)ui->table_upload->item(i, 0);
        if(item0->checkState() == Qt::Checked){
            //按钮状态  切换 发送信号
            QPushButton* button = (QPushButton*)ui->table_upload->cellWidget(i,5);
            if( button->text() == "暂停"){
                button->setText("开始");
                    // 信号 让核心类把结构体标志位置为暂停
                Q_EMIT SIG_setUploadPause( item0->m_info.timestamp, 1);
            }
        }
    }


}

void MainDialog::slot_uploadResume(bool flag)
{
    qDebug()<< "上传继续";
    //遍历列表 是否打钩
    int count = ui->table_upload->rowCount();
    for( int i = 0; i<count; ++i){
        MyTableWidgetItem* item0 = (MyTableWidgetItem*)ui->table_upload->item(i, 0);
        if(item0->checkState() == Qt::Checked){
            //按钮状态  切换 发送信号
            QPushButton* button = (QPushButton*)ui->table_upload->cellWidget(i,5);
            if( button->text() == "开始"){
                button->setText("暂停");
                Q_EMIT SIG_setUploadPause( item0->m_info.timestamp, 0);
            }
        }
    }
}

void MainDialog::slot_downloadPause(bool flag)
{
    qDebug()<< "下载暂停";
    //遍历列表 是否打钩
    int count = ui->table_download->rowCount();
    for( int i = 0; i<count; ++i){
        MyTableWidgetItem* item0 = (MyTableWidgetItem*)ui->table_download->item(i, 0);
        if(item0->checkState() == Qt::Checked){
            //按钮状态  切换 发送信号
            QPushButton* button = (QPushButton*)ui->table_download->cellWidget(i,5);
            if( button->text() == "暂停"){
                button->setText("开始");
                    // 信号 让核心类把结构体标志位置为暂停
                Q_EMIT SIG_setDownloadPause( item0->m_info.timestamp, 1);
            }
        }
    }
}

void MainDialog::slot_downloadResume(bool flag)
{
    qDebug()<< "下载继续";
    //遍历列表 是否打钩
    int count = ui->table_download->rowCount();
    for( int i = 0; i<count; ++i){
        MyTableWidgetItem* item0 = (MyTableWidgetItem*)ui->table_download->item(i, 0);
        if(item0->checkState() == Qt::Checked){
            //按钮状态  切换 发送信号
            QPushButton* button = (QPushButton*)ui->table_download->cellWidget(i,5);
            if( button->text() == "开始"){
                button->setText("暂停");
                    // 信号 让核心类把结构体标志位置为暂停
                Q_EMIT SIG_setDownloadPause( item0->m_info.timestamp, 0);
                qDebug() << "send SIG_setDownloadPause";
            }
        }
    }
}

//插入到上传中
void MainDialog::slot_insertUploadFile(FileInfo &info)
{
    //插入表格信息
    //列：文件 大小 时间 速度 进度 按钮
    //1.新增一行： 获取当前行+1 设置行数
    int rows = ui->table_upload->rowCount();
    ui->table_upload->setRowCount( rows+1 );

    //2. 设置该行的每一列--添加对象
    //文件名
    MyTableWidgetItem* item0 = new MyTableWidgetItem;  //不要定义对象，因为加到控件里，函数结束后对象的内容不能回收
                                                       //对象是栈区的，自动回收，new出来的在堆区
    item0->slot_setFileInfo( info );
    ui->table_upload->setItem( rows, 0, item0);
    //大小
    QTableWidgetItem* item1 = new QTableWidgetItem( FileInfo::getSize( info.size ) );
    ui->table_upload->setItem( rows, 1, item1);
    //时间
    QTableWidgetItem* item2 = new QTableWidgetItem( info.time );
    ui->table_upload->setItem( rows, 2, item2);
    //速度
    QTableWidgetItem* item3 = new QTableWidgetItem( "0KB" );
    ui->table_upload->setItem( rows, 3, item3);
    //进度条
    QProgressBar* progress = new QProgressBar;
    progress->setMaximum( info.size );
    ui->table_upload->setCellWidget( rows, 4, progress);
    //按钮
    QPushButton* button = new QPushButton;
    button->setFlat(1);
    if( info.isPause ){
        //button->setIcon( QIcon(":/images/pause.png"));
        button->setText( "开始" );
    }else{
        //button->setIcon( QIcon(":/images/play.png"));  /
        button->setText( "暂停" );
    }

    ui->table_upload->setCellWidget(rows, 5, button);  //QPushButton继承QWidget
}

void MainDialog::slot_insertUploadComplete(FileInfo &info)
{
    qDebug()<<__func__;
    //列：文件名 大小 时间 上传完成
    int rows = ui->table_complate->rowCount();
    ui->table_complate->setRowCount( rows+1 );
    //2. 设置该行的每一列--添加对象
    //文件名
    MyTableWidgetItem* item0 = new MyTableWidgetItem;
    item0->slot_setFileInfo( info );
    ui->table_complate->setItem( rows, 0, item0);
    //大小
    QTableWidgetItem* item1 = new QTableWidgetItem( FileInfo::getSize( info.size ) );
    ui->table_complate->setItem( rows, 1, item1);
    //时间
    QTableWidgetItem* item2 = new QTableWidgetItem( info.time );
    ui->table_complate->setItem( rows, 2, item2);
    //上传完成
    QTableWidgetItem* item3 = new QTableWidgetItem( "上传完成" );
    ui->table_complate->setItem( rows, 3, item3);
}

void MainDialog::slot_insertShareFileInfo(QString name, int size, QString time, int shareLink)
{
    //列：文件名 大小 时间 上传完成
    int rows = ui->table_share->rowCount();
    ui->table_share->setRowCount( rows+1 );
    //2. 设置该行的每一列--添加对象
    //文件名
    QTableWidgetItem* item0 = new QTableWidgetItem(name);
    ui->table_share->setItem( rows, 0, item0);
    //大小
    QTableWidgetItem* item1 = new QTableWidgetItem( FileInfo::getSize(size) );
    ui->table_share->setItem( rows, 1, item1);
    //时间
    QTableWidgetItem* item2 = new QTableWidgetItem( time );
    ui->table_share->setItem( rows, 2, item2);
    //分享码
    QTableWidgetItem* item3 = new QTableWidgetItem( QString::number(shareLink) );
    ui->table_share->setItem( rows, 3, item3);
}

void MainDialog::slot_updateUploadFileProgress(int timestamp, int pos)
{
    //遍历第0列
    int row = ui->table_upload->rowCount();  //rowCount会变， row不变
    for(int i = 0; i<row; ++i){
    //获取时间戳，判断是否同一文件
        MyTableWidgetItem* item0 = (MyTableWidgetItem*)ui->table_upload->item(i, 0);
        //一致
        if(item0->m_info.timestamp == timestamp){
            //更新进度
            QProgressBar* item4 = (QProgressBar*)ui->table_upload->cellWidget(i, 4);
            item4->setValue( pos );
            item0->m_info.pos = pos;
            //是否结束
            if( item4->value() >= item4->maximum() ){
            //是 删除这一项，添加到完成
                slot_insertUploadComplete( item0->m_info );
                slot_deleteUploadFileByRow( i );
            //return
                return;
            }
        }
    }
}

void MainDialog::slot_deleteUploadFileByRow(int row)
{
    qDebug()<<__func__;
    ui->table_upload->removeRow( row );
}

void MainDialog::slot_insertFileInfo(FileInfo &info)
{
    //列：文件名 大小 时间
    int rows = ui->table_file->rowCount();
    ui->table_file->setRowCount( rows+1 );
    //2. 设置该行的每一列--添加对象
    //文件名
    MyTableWidgetItem* item0 = new MyTableWidgetItem;
    item0->slot_setFileInfo( info );
    ui->table_file->setItem( rows, 0, item0);

    QString filesize;
    if(item0->m_info.type == "file"){
        filesize = FileInfo::getSize( info.size );
    }else{
        filesize = " ";
    }

    //大小
    QTableWidgetItem* item1 = new QTableWidgetItem( filesize );
    ui->table_file->setItem( rows, 1, item1);
    //时间
    QTableWidgetItem* item2 = new QTableWidgetItem( info.time );
    ui->table_file->setItem( rows, 2, item2);

}

void MainDialog::slot_insertDownloadFile(FileInfo &info)
{
    //插入表格信息
    //列：文件 大小 时间 速度 进度 按钮
    //1.新增一行： 获取当前行+1 设置行数
    int rows = ui->table_download->rowCount();
    ui->table_download->setRowCount( rows+1 );

    //2. 设置该行的每一列--添加对象
    //文件名
    MyTableWidgetItem* item0 = new MyTableWidgetItem;  //不要定义对象，因为加到控件里，函数结束后对象的内容不能回收
                                                       //对象是栈区的，自动回收，new出来的在堆区
    item0->slot_setFileInfo( info );
    ui->table_download->setItem( rows, 0, item0);
    //大小
    QTableWidgetItem* item1 = new QTableWidgetItem( FileInfo::getSize( info.size ) );
    ui->table_download->setItem( rows, 1, item1);
    //时间
    QTableWidgetItem* item2 = new QTableWidgetItem( info.time );
    ui->table_download->setItem( rows, 2, item2);
    //速度
    QTableWidgetItem* item3 = new QTableWidgetItem( "0KB" );
    ui->table_download->setItem( rows, 3, item3);
    //进度条
    QProgressBar* progress = new QProgressBar;
    progress->setMaximum( info.size );
    ui->table_download->setCellWidget( rows, 4, progress);
    //按钮
    QPushButton* button = new QPushButton;
    button->setFlat(1);
    if( info.isPause ){
       // button->setIcon( QIcon(":/images/pause.png"));  //
        button->setText( "开始" );
    }else{
        //button->setIcon( QIcon(":/images/play.png"));  //
        button->setText( "暂停" );
    }
    ui->table_download->setCellWidget(rows, 5, button);  //QPushButton继承QWidget

}

void MainDialog::slot_updateDownloadFileProgress(int timestamp, int pos)
{
    qDebug()<<__func__;
    //遍历第0列
    int row = ui->table_download->rowCount();  //rowCount会变， row不变
    for(int i = 0; i<row; ++i){
    //获取时间戳，判断是否同一文件
        MyTableWidgetItem* item0 = (MyTableWidgetItem*)ui->table_download->item(i, 0);
        //一致
        if(item0->m_info.timestamp == timestamp){
            //更新进度
            QProgressBar* item4 = (QProgressBar*)ui->table_download->cellWidget(i, 4);
            item4->setValue( pos );
            item0->m_info.pos = pos;
            //是否结束
            if( item4->value() >= item4->maximum() ){
            //是 删除这一项，添加到完成

                slot_insertDownloadComplete( item0->m_info );
                slot_deleteDownloadFileByRow( i );
            //return
                return;
            }
        }
    }
}

void MainDialog::slot_deleteDownloadFileByRow(int row)
{
    qDebug() <<__func__;
    ui->table_download->removeRow(row);
}

void MainDialog::slot_insertDownloadComplete(FileInfo &info)
{
    qDebug() <<__func__;
    //列：文件名 大小 时间 上传完成
    int rows = ui->table_complate->rowCount();
    ui->table_complate->setRowCount( rows+1 );
    //2. 设置该行的每一列--添加对象
    //文件名
    MyTableWidgetItem* item0 = new MyTableWidgetItem;
    item0->slot_setFileInfo( info );
    ui->table_complate->setItem( rows, 0, item0);
    //大小
    QTableWidgetItem* item1 = new QTableWidgetItem( FileInfo::getSize( info.size ) );
    ui->table_complate->setItem( rows, 1, item1);
    //时间
    QTableWidgetItem* item2 = new QTableWidgetItem( info.time );
    ui->table_complate->setItem( rows, 2, item2);
    //上传完成 点击打开文件夹
    //1.先创建一个按钮
    QPushButton* button = new QPushButton;
    //2. 绑定点击信号
    connect(button, SIGNAL(clicked(bool)), this, SLOT(slot_openPath(bool)));
    //3.设置图标
    button->setIcon(QIcon(":/images/folder.png"));
    button->setFlat( true );
    button->setToolTip( info.absolutePath );

    ui->table_complate->setCellWidget(rows, 3, button);
}
#include<QProcess>
void MainDialog::slot_openPath(bool flag)
{
    //获取不到目录，之前的方法，重写子类，继承父类并添加成员
    //这里只有一个要获取的信息，可以使用setToolTip
    //怎么获取按钮 QObject::sender() 可以获取发送信号的对象（按钮） 通过按钮获取setToolTip
    QPushButton* button = (QPushButton*)QObject::sender();
    QString path = button->toolTip();
    /*   /要转化成\\  */
    qDebug()<<"path:"<<path;
    path.replace('/','\\');
    qDebug()<<"path turn to:"<<path;
    //如何打开文件夹  文件资源管理器是一个进程
    //explorer /select,E:\QTcode\NetDisk\build-debug\debug\NetDisk\logo.ico
    //通过QT 打开进程
    QProcess process;
    QStringList lst; //两种添加方式
//    lst.push_back("/select,");
//    lst.push_back(path);"explorer",
    lst << QString("/select,") << path;
    qDebug()<<lst;
    process.startDetached( "explorer", lst);//参数是QSTringList
}

void MainDialog::slot_deleteAllFileInfo()
{
    //删除
    //ui->table_file->clear(); //删文字，行数不变 不用这个

    int rows = ui->table_file->rowCount();
    for( int i = rows; i>=0; --i){
        ui->table_file->removeRow(i);
    }
}

void MainDialog::slot_deleteAllShareInfo()
{
    int rows = ui->table_share->rowCount();
    for( int i = rows; i>=0; --i){
        ui->table_share->removeRow(i);
    }
}

//选中某一行，把一整行选中
void MainDialog::on_table_file_cellClicked(int row, int column)
{
    //获取选中行状态
    MyTableWidgetItem* item0 = (MyTableWidgetItem*)ui->table_file->item(row, 0);
    //切换勾选
    if( item0->checkState() == Qt::Checked ){
        item0->setCheckState( Qt::Unchecked );
    }else{
        item0->setCheckState( Qt::Checked );
    }
}

//右键弹出菜单 参数是相对位置，需要用绝对位置QCursor::pos()
void MainDialog::on_table_file_customContextMenuRequested(const QPoint &pos)
{
    qDebug()<<__func__;
    //弹出菜单
    m_menuFileInfo.exec( QCursor::pos() );
}

//路径跳转
void MainDialog:: on_table_file_cellDoubleClicked(int row, int column)
{
    //获取行列-->找到info.name 文件夹名
    MyTableWidgetItem * item0 = (MyTableWidgetItem*)ui->table_file->item( row, 0);
    //判断是否是文件夹 todo 不是文件夹打开
    if( item0->m_info.type != "file"){
    //设置路径 拼接 lb_path->text
        QString dir;
        dir = ui->lb_path->text() + item0->m_info.name + "/";
       ui->lb_path->setText( dir );
    //向kernel发送信号 刷新文件列表
       Q_EMIT SIG_changeDir(dir);
    }
}


void MainDialog::on_pb_prev_clicked()
{
    //获取当前目录
    QString dir = ui->lb_path->text();
    //判断 "/" 结束
    if(dir == "/")  return;
    //获取上一层目录，设置目录
    //先获取最后一个/的位置，取前面的所有字符
    dir = dir.left( dir.lastIndexOf("/") );
    dir = dir.left( dir.lastIndexOf("/")+1 ); //取到的结尾要有/ left参数是多少个
    qDebug()<<dir;
    ui->lb_path->setText( dir );
    //发送信号跳转路径
    Q_EMIT SIG_changeDir( dir );
}

void MainDialog::slot_getShare(bool)
{
    qDebug()<<__func__;
    //弹窗 获取分享码
    QString txt = QInputDialog::getText(this, "获取分享", "输入分享码");
    //过滤
    int code = txt.toInt();
    if( txt.length() != 9 || code < 100000000 || code >= 1000000000)
    {
        QMessageBox::about(this, "提示","分享码非法");
                return;
    }
    Q_EMIT SIG_getShareByLink(code, ui->lb_path->text() );
    //发信号 什么路径 添加什么分享码的文件
}


void MainDialog::on_table_upload_cellClicked(int row, int column)
{
    qDebug()<<__func__;
    //获取选中行状态
    MyTableWidgetItem* item0 = (MyTableWidgetItem*)ui->table_upload->item(row, 0);
    //切换勾选
    if( item0->checkState() == Qt::Checked ){
        item0->setCheckState( Qt::Unchecked );
    }else{
        item0->setCheckState( Qt::Checked );
    }
}


void MainDialog::on_table_download_cellClicked(int row, int column)
{
    qDebug()<<__func__;
    //获取选中行状态
    MyTableWidgetItem* item0 = (MyTableWidgetItem*)ui->table_download->item(row, 0);
    //切换勾选
    if( item0->checkState() == Qt::Checked ){
        item0->setCheckState( Qt::Unchecked );
    }else{
        item0->setCheckState( Qt::Checked );
    }
}

FileInfo& MainDialog::slot_getDownloadFileInfoByTimestamp(int timestamp)
{
    qDebug()<<__func__;
    //遍历第0列 相同的timestamp
    int rows = ui->table_download->rowCount();
    for( int i = 0; i<rows; +i){
        MyTableWidgetItem* item0 = (MyTableWidgetItem*)ui->table_download->item(i, 0);
        if(item0->m_info.timestamp == timestamp)
            return item0->m_info;
    }
}

FileInfo &MainDialog::slot_getUploadFileInfoByTimestamp(int timestamp)
{
    qDebug()<<__func__;
    //遍历第0列 相同的timestamp
    int rows = ui->table_upload->rowCount();
    for( int i = 0; i<rows; +i){
        MyTableWidgetItem* item0 = (MyTableWidgetItem*)ui->table_upload->item(i, 0);
        if(item0->m_info.timestamp == timestamp)
            return item0->m_info;
    }
}






