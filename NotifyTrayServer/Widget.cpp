#include "Widget.h"
#include "ui_Widget.h"

#include <QTimer>
#include <QTcpSocket>
#include <QDataStream>
#include <QMessageBox>
#include <QNetworkProxy>
#include <QProcess>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    haku();
}

Widget::~Widget()
{
    uninstallHook_NotifyWndProc();
    foreach(QTcpSocket *socket, mConnectionSet)
    {
        socket->close();
        socket->deleteLater();
    }
    if(mServer)
    {
        mServer->close();
    }
    delete ui;
}

///清理日志
void Widget::on_pushButton_ClearLog_clicked()
{
    ui->textBrowser->clear();
}

///新的连接
void Widget::slotNewConnection()
{
    logit(tr("发现新的客户端连接"));
    while(mServer->hasPendingConnections())
    {
        QTcpSocket *socket = mServer->nextPendingConnection();
//        socket->setProxy(QNetworkProxy::NoProxy);
        mConnectionSet.insert(socket);
        connect(socket, &QTcpSocket::readyRead, this, &Widget::slotReadyReadSocket, Qt::UniqueConnection);
        connect(socket, &QTcpSocket::disconnected, this, &Widget::slotDiscardSocket, Qt::UniqueConnection);
        logit(QString(tr("客户端[%1]已连接")).arg(socket->peerAddress().toString()));
    }
}

///读取socket
void Widget::slotReadyReadSocket()
{
    QTcpSocket *socket = reinterpret_cast<QTcpSocket*>(sender());
    QByteArray buffer;
    QDataStream socketStream(socket);
    socketStream.setVersion(QDataStream::Qt_5_12);
    socketStream.startTransaction();
    socketStream >> buffer;

    if(!socketStream.commitTransaction())
    {
        logit(QString(tr("等待客户端[%1]更多数据到达")).arg(socket->peerAddress().toString()));
        return;
    }

    QString header = buffer.mid(0, 128);
    QString fileType = header.split(",")[0].split(":")[1];
    buffer = buffer.mid(128);
    if(fileType == "message")
    {
        int appId = QString::fromStdString(buffer.toStdString()).toInt();
        openApp(appId);
        logit(QString(tr("打开程序 <%1>")).arg(mMSD[appId].appName));
    }
}

///断开socket
void Widget::slotDiscardSocket()
{
    QTcpSocket *socket = reinterpret_cast<QTcpSocket*>(sender());
    QSet<QTcpSocket*>::iterator it = mConnectionSet.find(socket);
    if(it != mConnectionSet.end())
    {
        logit(tr("一个客户端已断开"));
        mConnectionSet.remove(*it);
    }

    socket->deleteLater();
}

///初始化
void Widget::haku()
{
    scanTray();

    mDwMessage.insert(NIM_ADD,          "NIM_ADD");
    mDwMessage.insert(NIM_MODIFY,       "NIM_MODIFY");
    mDwMessage.insert(NIM_DELETE,       "NIM_DELETE");
    mDwMessage.insert(NIM_SETFOCUS,     "NIM_SETFOCUS");
    mDwMessage.insert(NIM_SETVERSION,   "NIM_SETVERSION");
    mMSD[IdWeChat].appName = tr("微信");
    mMSD[IdQQ].appName = tr("QQ");
    mMSD[IdCloudHub].appName = tr("云之家");
    mMSD[IdDingTalk].appName = tr("钉钉");

    logit(QString("安装钩子%1").arg(installHook_NotifyWndProc((HWND)this->winId())?tr("成功"):tr("失败")));

    mServer = new QTcpServer(this);
//    mServer->setProxy(QNetworkProxy::NoProxy);
    if(mServer->listen(QHostAddress::Any, 8080))
    {
        connect(mServer, &QTcpServer::newConnection, this, &Widget::slotNewConnection, Qt::UniqueConnection);
        logit(tr("服务已开启，监听客户端连接中..."));
    }
    else
    {
        logit(tr("无发开启服务"));
    }
}

///打印log
void Widget::logit(QString str)
{
    if(!ui->checkBox_Log->isChecked()) { return; }
    ui->textBrowser->append(QString("[%1] %2").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"), str));
}

///安装 Win32 钩子
bool Widget::installHook_NotifyWndProc(HWND hWnd)
{
    bool mCallWndProcHooked = false;
    HWND hShellWnd = ::FindWindow(TEXT("Shell_TrayWnd"), nullptr);
    if(!hShellWnd || !::IsWindow(hShellWnd))
    {
        logit(tr("无法找到托盘[Shell_TrayWnd]句柄"));
        return false;
    }

    mCallWndProcHooked = InstallCallWndProcHook(hWnd, hShellWnd);
    if(mCallWndProcHooked)
    {
        logit(tr("安装钩子[CallWndProc]成功"));
        return true;
    }
    else
    {
        logit(tr("安装钩子[CallWndProc]失败"));
        return false;
    }
}

///卸载 Win32 钩子
void Widget::uninstallHook_NotifyWndProc()
{
    UninstallCallWndProcHook();
    logit(tr("已卸载钩子[CallWndProc]"));
}

///发送socket消息
void Widget::sendMessage(QTcpSocket *socket, QString str)
{
    if(socket)
    {
        if(socket->isOpen())
        {
            QDataStream socketStream(socket);
            socketStream.setVersion(QDataStream::Qt_5_12);

            QByteArray header;
            header.prepend(QString("fileType:message,fileSize:%1;").arg(str.size()).toUtf8());
            header.resize(128);

            QByteArray byteArray = str.toUtf8();
            byteArray.prepend(header);

            socketStream.setVersion(QDataStream::Qt_5_12);
            socketStream << byteArray;
        }
        else
        {
            QMessageBox::critical(this, "错误", "Socket未打开");
        }
    }
    else
    {
        QMessageBox::critical(this, "错误", "Socket未连接");
    }
}

///广播socket消息
void Widget::broadcastMessage(QString str)
{
    foreach(QTcpSocket *socket, mConnectionSet)
    {
        sendMessage(socket, str);
    }
}

///处理图标信息
void Widget::taryInfo(QString type, ulong iconId, QString title, quint64 hWnd)
{
    logit(QString("type=%1, iconId=%3, title=%2").arg(type, title).arg(iconId));
    if(type == mDwMessage[NIM_MODIFY])
    {   //图标更改
        if(hWnd == mMSD[IdWeChat].hWnd)
        {
            checkFlash(IdWeChat, iconId);
        }
        else if(hWnd == mMSD[IdQQ].hWnd)
        {
            checkFlash(IdQQ, iconId);
        }
        else if(hWnd == mMSD[IdCloudHub].hWnd)
        {
            checkFlash(IdCloudHub, iconId);
        }
        else if(hWnd == mMSD[IdDingTalk].hWnd)
        {
            checkFlash(IdDingTalk, iconId);
        }
    }
    else if(type == mDwMessage[NIM_ADD] || type == mDwMessage[NIM_DELETE])
    {   //图标创建或销毁后，重新扫描图标
        QTimer::singleShot(1000, this, [this]{
            scanTray();
        });
    }
}

///检查软件
void Widget::checkFlash(int appId, ulong iconId)
{
    if(appId < IdWeChat || appId >= IdMax)
    {   //检查appId有效性，QQ的检测方法暂未实现
        return;
    }
    QDateTime datetime = QDateTime::currentDateTime();
    if(mMSD[appId].lastModifyTime.msecsTo(datetime) <= 1000 && mMSD[appId].lastIconId != iconId)
    {   //小于1000ms，且图标ID有变化
        mMSD[appId].modifyCount++;
    }
    else
    {
        mMSD[appId].modifyCount = 0;
    }
    mMSD[appId].lastModifyTime = datetime;
    mMSD[appId].lastIconId = iconId;
    //
    if(mMSD[appId].modifyCount >= 2)
    {   //2次有效则认为在闪动
        if(appId == IdQQ)
        {   //QQ单独处理
            broadcastMessage(QString("%1,1,%2").arg(appId).arg(mMSD[IdQQ].lastHasIcon));
            mMSD[IdQQ].lastHasIcon = !mMSD[IdQQ].lastHasIcon;
        }
        else
        {   //微信、云之家、钉钉
            broadcastMessage(QString("%1,1,%2").arg(appId).arg(iconId != 0));
        }
    }
    else
    {
        broadcastMessage(QString("%1,0,0").arg(appId));
    }
}

///扫描图标句柄
void Widget::scanTray()
{
    QProcess process;

    QString program = "ScanTray.exe";
    process.start(program);
    //等待外部程序执行完成或者设置最大等待时间
    if(!process.waitForFinished(5000))
    {
        logit(tr("扫描图标句柄超时!"));
        return;
    }
    QByteArray result = process.readAll();
    QString output = QString::fromLocal8Bit(result);

    QStringList tarys = output.split(";;HAKU_LINE;;", QString::SkipEmptyParts);
    for(int i = IdWeChat; i <= IdDingTalk; i++)
    {
        mMSD[i].run = false;
    }
    foreach(QString tary, tarys)
    {
        QStringList strs;
        bool ok;
        if(tary.contains("微信") && tary.contains("WeChat.exe"))
        {
            strs = tary.split(";;HAKU_ITEM;;");
            mMSD[IdWeChat].run = true;
            mMSD[IdWeChat].hWnd = QString("0x%1").arg(strs.at(0)).toUInt(&ok, 16);
            mMSD[IdWeChat].appPath = strs.at(1);
            logit(tr("已获取到 <%1> 句柄[0x%2]").arg(tr("微信"), QString::number(mMSD[IdWeChat].hWnd, 16)));
        }
        else if(tary.contains("QQ.exe"))
        {
            strs = tary.split(";;HAKU_ITEM;;");
            mMSD[IdQQ].run = true;
            mMSD[IdQQ].hWnd = QString("0x%1").arg(strs.at(0)).toUInt(&ok, 16);
            mMSD[IdQQ].appPath = strs.at(1);
            logit(tr("已获取到 <%1> 句柄[0x%2]").arg(tr("QQ"), QString::number(mMSD[IdQQ].hWnd, 16)));
        }
        else if(tary.contains("云之家") && tary.contains("CloudHub.exe"))
        {
            strs = tary.split(";;HAKU_ITEM;;");
            mMSD[IdCloudHub].run = true;
            mMSD[IdCloudHub].hWnd = QString("0x%1").arg(strs.at(0)).toUInt(&ok, 16);
            mMSD[IdCloudHub].appPath = strs.at(1);
            logit(tr("已获取到 <%1> 句柄[0x%2]").arg(tr("云之家"), QString::number(mMSD[IdCloudHub].hWnd, 16)));
        }
        else if(tary.contains("DingTalk.exe"))   //钉钉消息闪动时，没有"钉钉"文本
        {
            strs = tary.split(";;HAKU_ITEM;;");
            mMSD[IdDingTalk].run = true;
            mMSD[IdDingTalk].hWnd = QString("0x%1").arg(strs.at(0)).toUInt(&ok, 16);
            mMSD[IdDingTalk].appPath = strs.at(1);
            logit(tr("已获取到 <%1> 句柄[0x%2]").arg(tr("钉钉"), QString::number(mMSD[IdDingTalk].hWnd, 16)));
        }
    }
    for(int i = IdWeChat; i <= IdDingTalk; i++)
    {
        if(!mMSD[i].run)
        {
            mMSD[i].hWnd = 0x0;
        }
    }
}

///显示窗口并置顶
void Widget::openApp(int appId)
{
    if(appId == IdQQ)
    {   //由于QQ可打开多例，故暂未实现
        return;
    }

    HWND hWnd = ::FindWindowA(nullptr, mMSD[appId].appName.toLocal8Bit());

    QProcess progress;
    progress.start(mMSD[appId].appPath);
    if(!progress.waitForFinished())
    {
        logit(QString("打开 <%1> 超时").arg(mMSD[appId].appName));
        return;
    }

    if(!hWnd || !::IsWindow(hWnd))
    {
        logit(QString("无法找到 <%1> 的窗口").arg(mMSD[appId].appName));
        return;
    }
    //钉钉在虚拟机全屏后，无法置顶窗口，只有托盘弹出的窗口可置顶
    ::ShowWindow(hWnd, SW_SHOWNORMAL);
    ::SetForegroundWindow(hWnd);
}

bool Widget::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType)
    Q_UNUSED(result)

    MSG* pMsg = reinterpret_cast<MSG*>(message);
    switch(pMsg->message)
    {
    case WM_COPYDATA:
    {
        COPYDATASTRUCT *lpReceiveCDS = reinterpret_cast<COPYDATASTRUCT *>(pMsg->lParam);
        PTRAY_ICON_DATAW lpNotifyData = nullptr;
        if(lpReceiveCDS == nullptr || IsBadReadPtr(lpReceiveCDS, sizeof(TRAY_ICON_DATAW)) == TRUE)
        {
            break;
        }
//        logit(QString("lpReceiveCDS->cbData=%1, sizeof(TRAY_ICON_DATAW)=%2").arg(lpReceiveCDS->cbData).arg(sizeof(TRAY_ICON_DATAW)));
        if(lpReceiveCDS->dwData == WM_NotifyCallWndProc && lpReceiveCDS->cbData == sizeof(TRAY_ICON_DATAW))
        {
            lpNotifyData = (TRAY_ICON_DATAW *)lpReceiveCDS->lpData;
            logit(QString("[%1] => [HWND: 0x%2]").arg(mDwMessage[lpNotifyData->dwMessage], QString::number(pMsg->wParam, 16)));
            taryInfo(mDwMessage[lpNotifyData->dwMessage], lpNotifyData->uIconID, QString::fromWCharArray(lpNotifyData->szTip), pMsg->wParam);
        }
    }
        break;
    default:
        break;
    }

    return QWidget::nativeEvent(eventType, message, result);
}
