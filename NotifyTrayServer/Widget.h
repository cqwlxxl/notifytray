#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMap>
#include <QTcpServer>
#include <QDateTime>

#include <windows.h>
#include "../LibTrayHook/LibTrayHook/trayhook.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

enum AppId
{
    IdWeChat = 0,       //微信
    IdQQ,               //QQ
    IdCloudHub,         //云之家
    IdDingTalk,         //钉钉
    IdMax               //分界线
};

struct ModifySocketData
{
    QString     appName;
    QDateTime   lastModifyTime {QDateTime::currentDateTime()};
    ulong       lastIconId {0};
    quint64     hWnd {0x0};             //句柄
    int         modifyCount {0};
};

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    Ui::Widget *ui;

private slots:
    void slotNewConnection();       //新的连接
    void slotDiscardSocket();       //断开socket

private:
    void haku();                                        //初始化
    void logit(QString str);                            //打印log
    bool installHook_NotifyWndProc(HWND hWnd);          //安装 Win32 钩子
    void uninstallHook_NotifyWndProc();                 //卸载 Win32 钩子
    void sendMessage(QTcpSocket* socket, QString str);  //发送socket消息
    void broadcastMessage(QString str);                 //广播socket消息
    void taryInfo(QString type, ulong iconId, QString title, quint64 hWnd);         //处理图标信息
    void checkFlash(int appId, ulong iconId);                       //检查是否闪动
    void scanTray();                                    //扫描图标句柄

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, long *result);

private:
    QMap<int, QString>      mDwMessage;
    QTcpServer              *mServer;
    QSet<QTcpSocket*>       mConnectionSet;
    ModifySocketData        mMSD[IdMax];
};
#endif // WIDGET_H
