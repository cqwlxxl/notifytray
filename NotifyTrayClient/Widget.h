#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTcpSocket>

#include "TopBar.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    Ui::Widget *ui;

private slots:
    void on_pushButton_LinkServer_clicked();        //连接服务器
    void on_pushButton_ClearLog_clicked();          //清理日志

private slots:
    void slotReadSocket();          //读取socket
    void slotDiscardSocket();       //断开socket

private:
    void haku();                //初始化
    void logit(QString str);    //打印log
    void syncIcon(int appId, bool hasMsg, bool hasIcon);    //同步icon
    void setLinked(bool link);  //设置是否连上服务器

private:
    QTcpSocket  *mSocket {nullptr};
    TopBar      *mTopBar {new TopBar(nullptr)};     //桌面置顶条
    bool        mLinked {false};                    //是否连接上服务器
    QString     mIps[3];                            //历史IP
};
#endif // WIDGET_H
