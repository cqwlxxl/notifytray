#include "Widget.h"
#include "ui_Widget.h"

#include <QDateTime>
#include <QHostAddress>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkProxy>
#include <QRegExp>
#include <QRegExpValidator>
#include <QSettings>
#include <QTextCodec>

#define CONFIG_FILE_PATH    "src/config/config.ini"     //配置文件路径

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    haku();
}

Widget::~Widget()
{
    delete mTopBar;
    mTopBar = nullptr;
    if(mSocket)
    {
        if(mSocket->isOpen())
        {
            mSocket->close();
        }
    }

    delete ui;
}

///连接服务器
void Widget::on_pushButton_LinkServer_clicked()
{
    if(mLinked)
    {   //已连接
        if(mSocket)
        {
            if(mSocket->isOpen())
            {
                mSocket->close();
            }
        }
    }
    else
    {   //未连接
        QString ip = ui->comboBox_ServerIp->currentText().trimmed();
        QRegExp regExp(R"(\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\b)");
        if(!regExp.exactMatch(ip))
        {
            QMessageBox::information(this, tr("IP不合法"), tr("请输入正确的IP"));
            return;
        }
        //判断是否是新IP
        if(ip != ui->comboBox_ServerIp->itemText(0))
        {
            mIps[2] = mIps[1];
            mIps[1] = mIps[0];
            mIps[0] = ip;
            ui->comboBox_ServerIp->setItemText(2, mIps[2]);
            ui->comboBox_ServerIp->setItemText(1, mIps[1]);
            ui->comboBox_ServerIp->setItemText(0, mIps[0]);
            QSettings ini(CONFIG_FILE_PATH, QSettings::IniFormat);
            ini.setIniCodec(QTextCodec::codecForName("utf-8"));     //设置文本编码
            ini.setValue("history_ip/ip0", mIps[0]);
            ini.setValue("history_ip/ip1", mIps[1]);
            ini.setValue("history_ip/ip2", mIps[2]);
        }
        ui->comboBox_ServerIp->setCurrentText(ip);
        int port = ui->lineEdit_ServerPort->text().trimmed().toInt();
        if(port < 0 || port > 65535)
        {
            QMessageBox::information(this, tr("端口不合法"), tr("请输入正确的端口(0~65535)"));
            return;
        }

        if(!mSocket)
        {
            mSocket = new QTcpSocket(this);
            connect(mSocket, &QTcpSocket::readyRead, this, &Widget::slotReadSocket, Qt::UniqueConnection);
            connect(mSocket, &QTcpSocket::disconnected, this, &Widget::slotDiscardSocket, Qt::UniqueConnection);
        }
        mSocket->connectToHost(ip, port);
    //    mSocket->setProxy(QNetworkProxy::NoProxy);

        if(mSocket->waitForConnected(5000))
        {
            logit(tr("已连接服务器"));
            setLinked(true);
        }
        else
        {
            if(mSocket)
            {
                delete mSocket;
                mSocket = nullptr;
            }
            QMessageBox::information(this, tr("错误"), tr("无法连接服务器"));
        }
    }
}

///清理日志
void Widget::on_pushButton_ClearLog_clicked()
{
    ui->textBrowser->clear();
}

///读取socket
void Widget::slotReadSocket()
{
    QByteArray buffer;

    QDataStream socketStream(mSocket);
    socketStream.setVersion(QDataStream::Qt_5_12);

    socketStream.startTransaction();
    socketStream >> buffer;

    if(!socketStream.commitTransaction())
    {
        logit(QString("[%1/%2] 等待数据传输完成").arg(mSocket->peerAddress().toString()).arg(mSocket->socketDescriptor()));
        return;
    }

    QString header = buffer.mid(0, 128);
    QString fileType = header.split(",")[0].split(":")[1];

    buffer = buffer.mid(128);

    if(fileType == "message")
    {
        QString msg = QString::fromStdString(buffer.toStdString());
        QStringList values = msg.split(",");
        syncIcon(values[0].toInt(), values[1]=="1", values[2]=="1");
        logit(QString("[%1/%3] %2").arg(mSocket->peerAddress().toString(), msg).arg(mSocket->socketDescriptor()));
    }
}

///断开socket
void Widget::slotDiscardSocket()
{
    setLinked(false);
    mSocket->deleteLater();
    mSocket = nullptr;
    logit("已断开服务器");
}

#ifdef Q_OS_WIN
///打开微信
void Widget::slotTrayActive_WeChat(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Trigger)
    {   //单击图标
        sendMessage(QString::number(IdWeChat));
    }
}

///打开QQ
void Widget::slotTrayActive_QQ(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Trigger)
    {   //单击图标
        sendMessage(QString::number(IdQQ));
    }
}

///打开云之家
void Widget::slotTrayActive_CloudHub(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Trigger)
    {   //单击图标
        sendMessage(QString::number(IdCloudHub));
    }
}

///打开钉钉
void Widget::slotTrayActive_DingTalk(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Trigger)
    {   //单击图标
        sendMessage(QString::number(IdDingTalk));
    }
}
#endif

///打开微信
void Widget::slotTrayMenuAction_WeChat()
{
    sendMessage(QString::number(IdWeChat));
}

///打开QQ
void Widget::slotTrayMenuAction_QQ()
{
    sendMessage(QString::number(IdQQ));
}

///打开云之家
void Widget::slotTrayMenuAction_CloudHub()
{
    sendMessage(QString::number(IdCloudHub));
}

///打开钉钉
void Widget::slotTrayMenuAction_DingTalk()
{
    sendMessage(QString::number(IdDingTalk));
}

///初始化
void Widget::haku()
{
    QSettings ini(CONFIG_FILE_PATH, QSettings::IniFormat);
    ini.setIniCodec(QTextCodec::codecForName("utf-8"));     //设置文本编码
    mIps[0] = ini.value("history_ip/ip0", "").toString();
    mIps[1] = ini.value("history_ip/ip1", "").toString();
    mIps[2] = ini.value("history_ip/ip2", "").toString();
    ui->comboBox_ServerIp->setItemText(0, mIps[0]);
    ui->comboBox_ServerIp->setItemText(1, mIps[1]);
    ui->comboBox_ServerIp->setItemText(2, mIps[2]);

    ui->label_IconWeChat->setVisible(false);
    ui->label_IconQQ->setVisible(false);
    ui->label_IconCloudHub->setVisible(false);
    ui->label_IconDingTalk->setVisible(false);
    ui->label_IconWeChat->setScaledContents(true);      //设置图片自动缩放
    ui->label_IconWeChat->setPixmap(QPixmap("src/icon/WeChat_24x.png"));
    ui->label_IconQQ->setScaledContents(true);
    ui->label_IconQQ->setPixmap(QPixmap("src/icon/QQ_24x.png"));
    ui->label_IconCloudHub->setScaledContents(true);
    ui->label_IconCloudHub->setPixmap(QPixmap("src/icon/CloudHub_24x.png"));
    ui->label_IconDingTalk->setScaledContents(true);
    ui->label_IconDingTalk->setPixmap(QPixmap("src/icon/DingTalk_24x.png"));

    haku_tray();
}

///初始化托盘
void Widget::haku_tray()
{
    mTray[IdWeChat] = new QSystemTrayIcon(this);
    mTray[IdQQ] = new QSystemTrayIcon(this);
    mTray[IdCloudHub] = new QSystemTrayIcon(this);
    mTray[IdDingTalk] = new QSystemTrayIcon(this);
    mTray[IdWeChat]->setIcon(QIcon(QPixmap("src/icon/WeChat_24x.png")));
    mTray[IdQQ]->setIcon(QIcon(QPixmap("src/icon/QQ_24x.png")));
    mTray[IdCloudHub]->setIcon(QIcon(QPixmap("src/icon/CloudHub_24x.png")));
    mTray[IdDingTalk]->setIcon(QIcon(QPixmap("src/icon/DingTalk_24x.png")));
#ifdef Q_OS_WIN
    connect(mTray[IdWeChat], &QSystemTrayIcon::activated, this, &Widget::slotTrayActive_WeChat, Qt::UniqueConnection);
    connect(mTray[IdQQ], &QSystemTrayIcon::activated, this, &Widget::slotTrayActive_QQ, Qt::UniqueConnection);
    connect(mTray[IdCloudHub], &QSystemTrayIcon::activated, this, &Widget::slotTrayActive_CloudHub, Qt::UniqueConnection);
    connect(mTray[IdDingTalk], &QSystemTrayIcon::activated, this, &Widget::slotTrayActive_DingTalk, Qt::UniqueConnection);
#endif

    mMenu[IdWeChat] = new QMenu(this);
    mAction[IdWeChat] = new QAction(this);
    mAction[IdWeChat]->setText(tr("打开微信"));
    connect(mAction[IdWeChat], &QAction::triggered, this, &Widget::slotTrayMenuAction_WeChat, Qt::UniqueConnection);
    mTray[IdWeChat]->setContextMenu(mMenu[IdWeChat]);
    mMenu[IdWeChat]->addAction(mAction[IdWeChat]);

    mMenu[IdQQ] = new QMenu(this);
    mAction[IdQQ] = new QAction(this);
    mAction[IdQQ]->setText(tr("打开QQ"));
    connect(mAction[IdQQ], &QAction::triggered, this, &Widget::slotTrayMenuAction_QQ, Qt::UniqueConnection);
    mTray[IdQQ]->setContextMenu(mMenu[IdQQ]);
    mMenu[IdQQ]->addAction(mAction[IdQQ]);

    mMenu[IdCloudHub] = new QMenu(this);
    mAction[IdCloudHub] = new QAction(this);
    mAction[IdCloudHub]->setText(tr("打开云之家"));
    connect(mAction[IdCloudHub], &QAction::triggered, this, &Widget::slotTrayMenuAction_CloudHub, Qt::UniqueConnection);
    mTray[IdCloudHub]->setContextMenu(mMenu[IdCloudHub]);
    mMenu[IdCloudHub]->addAction(mAction[IdCloudHub]);

    mMenu[IdDingTalk] = new QMenu(this);
    mAction[IdDingTalk] = new QAction(this);
    mAction[IdDingTalk]->setText(tr("打开钉钉"));
    connect(mAction[IdDingTalk], &QAction::triggered, this, &Widget::slotTrayMenuAction_DingTalk, Qt::UniqueConnection);
    mTray[IdDingTalk]->setContextMenu(mMenu[IdDingTalk]);
    mMenu[IdDingTalk]->addAction(mAction[IdDingTalk]);

}

///打印log
void Widget::logit(QString str)
{
    if(!ui->checkBox_Log->isChecked()) { return; }
    ui->textBrowser->append(QString("[%1] %2").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"), str));
}

///同步icon
void Widget::syncIcon(int appId, bool hasMsg, bool hasIcon)
{
    mTopBar->Hi();
    mTopBar->SyncIcon(appId, hasMsg, hasIcon);

    switch(appId)
    {
    case IdWeChat:
        if(hasIcon)
        {
            ui->label_IconWeChat->setPixmap(QPixmap("src/icon/WeChat_24x.png"));
            mTray[IdWeChat]->setIcon(QIcon(QPixmap(":/Res/icon/WeChat_24x.png")));
        }
        else
        {
            ui->label_IconWeChat->clear();
            mTray[IdWeChat]->setIcon(QIcon(QPixmap(":/Res/icon/NoIcon_24x.png")));
        }
        ui->label_IconWeChat->setVisible(hasMsg);
        mTray[IdWeChat]->setVisible(hasMsg);
        break;
    case IdQQ:
        if(hasIcon)
        {
            ui->label_IconQQ->setPixmap(QPixmap("src/icon/QQ_24x.png"));
            mTray[IdQQ]->setIcon(QIcon(QPixmap(":/Res/icon/QQ_24x.png")));
        }
        else
        {
            ui->label_IconQQ->clear();
            mTray[IdQQ]->setIcon(QIcon(QPixmap(":/Res/icon/NoIcon_24x.png")));
        }
        ui->label_IconQQ->setVisible(hasMsg);
        mTray[IdQQ]->setVisible(hasMsg);
        break;
    case IdCloudHub:
        if(hasIcon)
        {
            ui->label_IconCloudHub->setPixmap(QPixmap("src/icon/CloudHub_24x.png"));
            mTray[IdCloudHub]->setIcon(QIcon(QPixmap(":/Res/icon/CloudHub_24x.png")));
        }
        else
        {
            ui->label_IconCloudHub->clear();
            mTray[IdCloudHub]->setIcon(QIcon(QPixmap(":/Res/icon/NoIcon_24x.png")));
        }
        ui->label_IconCloudHub->setVisible(hasMsg);
        mTray[IdCloudHub]->setVisible(hasMsg);
        break;
    case IdDingTalk:
        if(hasIcon)
        {
            ui->label_IconDingTalk->setPixmap(QPixmap("src/icon/DingTalk_24x.png"));
            mTray[IdDingTalk]->setIcon(QIcon(QPixmap(":/Res/icon/DingTalk_24x.png")));
        }
        else
        {
            ui->label_IconDingTalk->clear();
            mTray[IdDingTalk]->setIcon(QIcon(QPixmap(":/Res/icon/NoIcon_24x.png")));
        }
        ui->label_IconDingTalk->setVisible(hasMsg);
        mTray[IdDingTalk]->setVisible(hasMsg);
        break;
    default:
        break;
    }
}

///设置是否连上服务器
void Widget::setLinked(bool link)
{
    mLinked = link;
    //设置控件
    ui->comboBox_ServerIp->setEnabled(!link);
    ui->lineEdit_ServerPort->setEnabled(!link);
    ui->pushButton_LinkServer->setText(link?tr("断开"):tr("连接服务器"));
    if(!link)
    {
        mTopBar->Bye();
        ui->label_IconWeChat->setVisible(false);
        ui->label_IconQQ->setVisible(false);
        ui->label_IconCloudHub->setVisible(false);
        ui->label_IconDingTalk->setVisible(false);
        mTray[IdWeChat]->setVisible(false);
        mTray[IdQQ]->setVisible(false);
        mTray[IdCloudHub]->setVisible(false);
        mTray[IdDingTalk]->setVisible(false);
    }
}

///发送socket
void Widget::sendMessage(QString msg)
{
    if(mSocket)
    {
        if(mSocket->isOpen())
        {
            QDataStream socketStream(mSocket);
            socketStream.setVersion(QDataStream::Qt_5_12);

            QByteArray header;
            header.prepend(QString("fileType:message,fileSize:%1;").arg(msg.size()).toUtf8());
            header.resize(128);

            QByteArray byteArray = msg.toUtf8();
            byteArray.prepend(header);

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
