#include "logindialog.h"
#include "ui_logindialog.h"
#include<QMessageBox>

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);

    setWindowTitle("登录&注册");
    //默认登录窗口
    ui->tw_page->setCurrentIndex(0);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::on_pb_register_clicked()
{
    //注册信息采集
    QString tel = ui->le_tel_register->text();
    QString password = ui->le_password_register->text();
    QString comfirm = ui->le_confirm_register->text();
    QString name = ui->le_name_register->text();

    QString tmpName = name;  //判断姓名是否都是空格，用remove(" ")会影响原字符串
    //过滤
    //输入是否为空
    if(tel.isEmpty() || password.isEmpty() || comfirm.isEmpty()
            || name.isEmpty() || tmpName.remove(" ").isEmpty())
    {
        QMessageBox::about(this, "提示", "不可为空");
        return ;
    }
    //手机号是否合法  -- 正则表达式
    QRegExp exp( "^1[346789][0-9]\{9\}$"); //9位{9} ^以什么开头 $以什么结尾
    bool res = exp.exactMatch( tel );
    if( !res )
    {
        QMessageBox::about(this, "提示", "手机号不合法");
        return;
    }
    //密码是否过长
    if( tel.size() > 20)
    {
        QMessageBox::about(this, "提示", "密码过长，长度小于20");
        return;
    }
    //确认密码一致
    if( comfirm != password )
    {
        QMessageBox::about(this, "提示", "两次输入的密码不一致");
        return;
    }
    //昵称是否过长  敏感词汇过滤-->加载一个配置文件 正则匹配
    if( name.size() > 10)
    {
        QMessageBox::about(this, "提示", "昵称过长，长度小于10");
        return;
    }
    //发送信号
    Q_EMIT SIG_registerCommit(tel, password, name);
}




void LoginDialog::on_pb_login_clicked()
{
    //登录信息采集
    QString tel = ui->le_tel->text();
    QString password = ui->le_password->text();

    //过滤
    //输入是否为空
    if(tel.isEmpty() || password.isEmpty() )
    {
        QMessageBox::about(this, "提示", "不可为空");
        return ;
    }
    //手机号是否合法  -- 正则表达式
    QRegExp exp( "^1[346789][0-9]\{9\}$"); //9位{9} ^以什么开头 $以什么结尾
    bool res = exp.exactMatch( tel );
    if( !res )
    {
        QMessageBox::about(this, "提示", "手机号不合法");
        return;
    }
    //密码是否过长
    if( tel.size() > 20)
    {
        QMessageBox::about(this, "提示", "密码过长，长度小于20");
        return;
    }

    //发送信号
    Q_EMIT SIG_loginCommit(tel, password);
}
//注册界面清空
void LoginDialog::on_pb_clear_register_clicked()
{
    ui->le_tel_register->setText("");
    ui->le_name_register->setText("");
    ui->le_confirm_register->setText("");
    ui->le_password_register->setText("");
}

//登录界面清空
void LoginDialog::on_pb_clear_clicked()
{
    ui->le_tel->setText("");
    ui->le_password->setText("");
}

