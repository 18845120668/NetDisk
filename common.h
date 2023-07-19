#ifndef COMMON_H
#define COMMON_H

#include<QString>
#include<QDebug>
////////////////////文件信息/////////////////
struct FileInfo
{
    FileInfo():fileid(0) , size(0),pFile( nullptr )
      , pos(0) , isPause(0), timestamp(0){

    }
    int fileid;
    QString name;
    QString dir;  //网盘的目录
    QString time;
    int size;   //int是32位 最大值2G，使用64位可能出现字节对齐的问题，设置网盘文件小于2G（大文件可以压缩、分割）
    QString md5;
    QString type;
    QString absolutePath;   //本地文件的绝对路径

    int pos; //上传或下载到什么位置
    int isPause; //暂停  0 1
    int timestamp;
    //文件指针
    FILE* pFile;

    //字节单位换算
    static QString getSize( int size )  //返回Kb Mb
    {
        QString res;
        int count = 0;
        int tmp = size;
        while (tmp != 0) {
            tmp /= 1024;
            if( tmp != 0) ++count;
        }
     //  qDebug()<< count<<" " <<size;
        switch( count )
        {
        case 0: //0.xxKB
            res = QString("0.%1KB").arg( (int)(size%1024/1024.0*100), 2, 10, QChar('0')); //arg(, 多宽，进制，不够位宽缺省的字符 )
            if(size !=0 && res == "0.00KB"){ //文件太小
                res == "0.01KB";
            }
            break;
        case 1:
            res = QString("%1.%2KB").arg( size/1024 ).arg( (int)(size%1024/1024.0*100), 2, 10, QChar('0') );
            break;
        case 2:   //MB
            res = QString("%1.%2MB").arg( size/1024/1024).arg((int)(size/1024%1024/1024.0*100), 2, 10,QChar('0') );
            break;
        case 3: //GB
            res = QString("%1.%2GB").arg(size/1024/1024).arg( (int)(size/1024/1024%1024/1024.0*100),2,10,QChar('0'));
            break;
        default: //过大
            break;
        }
        return res;
    }
};



#endif // COMMON_H
