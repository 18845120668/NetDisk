#ifndef MYTABLEWIDGETITEM_H
#define MYTABLEWIDGETITEM_H

#include <QTableWidgetItem>
#include"common.h"

class MainDialog;
class MyTableWidgetItem : public QTableWidgetItem
{
//    Q_OBJECT
public:
    MyTableWidgetItem();
public slots:
    void slot_setFileInfo( FileInfo& info);
private:
    //设置文件信息
    FileInfo m_info;
    friend class MainDialog;
};

#endif // MYTABLEWIDGETITEM_H
