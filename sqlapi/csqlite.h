#ifndef CSQLITE_H
#define CSQLITE_H

#include<QStringList>
#include<QSqlDatabase>
#include<QSqlQuery>
#include<QSqlDriver>
#include<QSqlRecord>
#include<QSqlError>
#include<QMutex>
#include<QTextCodec>
#include<QDebug>

class CSqlite
{
public:
    CSqlite();
    ~CSqlite();

    void ConnectSql(QString db);  //只需要一个绝对路径 找到文件
    void DisConnect(); //关闭文件

    //查询
    bool SelectSql(QString sqlStr , int nColumn , QStringList & list);

    //更新: 删除, 插入 , 修改
    bool UpdateSql(QString sqlStr);

private:

    QSqlDatabase m_db;
    QMutex m_mutex;
};

#endif // CSQLITE_H
