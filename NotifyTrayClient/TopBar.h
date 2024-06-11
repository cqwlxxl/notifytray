#ifndef TOPBAR_H
#define TOPBAR_H

#include <QWidget>

enum AppId
{
    IdWeChat = 0,       //微信
    IdQQ,               //QQ
    IdCloudHub,         //云之家
    IdDingTalk,         //钉钉
    IdMax               //分界线
};

namespace Ui {
class TopBar;
}

class TopBar : public QWidget
{
    Q_OBJECT

public:
    explicit TopBar(QWidget *parent = nullptr);
    ~TopBar();

private:
    Ui::TopBar *ui;

private slots:
    void slotWorkAreaResized(const QRect &geometry);        //显示器大小改变

public:
    void Hi();          //显示窗口
    void Bye();         //隐藏窗口
    void SyncIcon(int appId, bool hasMsg, bool hasIcon);    //同步icon
};

#endif // TOPBAR_H
