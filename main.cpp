#include "maindialog.h"

#include <QApplication>
#include"ckernel.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
 //   MainDialog w;
 //   w.show();  在主窗口不会退出 对象一致不销毁 在kernel里如果用对象 会销毁需要new一个堆区的量
    CKernel::GetInstance();
    return a.exec();
}
