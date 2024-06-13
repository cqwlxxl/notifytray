#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTcpSocket>
#include <QSystemTrayIcon>

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
#ifdef Q_OS_WIN
    void slotTrayActive_WeChat(QSystemTrayIcon::ActivationReason reason);       //打开微信
    void slotTrayActive_QQ(QSystemTrayIcon::ActivationReason reason);           //打开QQ
    void slotTrayActive_CloudHub(QSystemTrayIcon::ActivationReason reason);     //打开云之家
    void slotTrayActive_DingTalk(QSystemTrayIcon::ActivationReason reason);     //打开钉钉
#endif
    void slotTrayMenuAction_WeChat();       //打开微信
    void slotTrayMenuAction_QQ();           //打开QQ
    void slotTrayMenuAction_CloudHub();     //打开云之家
    void slotTrayMenuAction_DingTalk();     //打开钉钉

private:
    void haku();                //初始化
    void haku_tray();           //初始化托盘
    void logit(QString str);    //打印log
    void syncIcon(int appId, bool hasMsg, bool hasIcon);    //同步icon
    void setLinked(bool link);  //设置是否连上服务器
    void sendMessage(QString msg);                          //发送socket

private:
    QTcpSocket  *mSocket {nullptr};
    TopBar      *mTopBar {new TopBar(nullptr)};     //桌面置顶条
    bool        mLinked {false};                    //是否连接上服务器
    QString     mIps[3];                            //历史IP

    QSystemTrayIcon     *mTray[IdMax];      //托盘图标
    QMenu               *mMenu[IdMax];      //托盘菜单
    QAction             *mAction[IdMax];    //托盘菜单动作
};
#endif // WIDGET_H
