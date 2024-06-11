#include "TopBar.h"
#include "ui_TopBar.h"

#include <QScreen>

TopBar::TopBar(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TopBar)
{
    ui->setupUi(this);

    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setAttribute(Qt::WA_TransparentForMouseEvents);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::SubWindow | Qt::WindowStaysOnTopHint);       //无边框|子窗口|置顶
    this->move((qApp->primaryScreen()->size().width()-this->width())/2, 0);   //顶部居中
    ui->label_IconWeChat->setVisible(false);
    ui->label_IconQQ->setVisible(false);
    ui->label_IconCloudHub->setVisible(false);
    ui->label_IconDingTalk->setVisible(false);
    connect(QApplication::screens().at(0), &QScreen::availableGeometryChanged, this, &TopBar::slotWorkAreaResized, Qt::UniqueConnection);
}

TopBar::~TopBar()
{
    delete ui;
}

///显示器大小改变
void TopBar::slotWorkAreaResized(const QRect &geometry)
{
    Q_UNUSED(geometry)
    this->move((qApp->primaryScreen()->size().width()-this->width())/2, 0);   //顶部居中
}

///显示窗口
void TopBar::Hi()
{
    this->show();
}

///隐藏窗口
void TopBar::Bye()
{
    ui->label_IconWeChat->setVisible(false);
    ui->label_IconQQ->setVisible(false);
    ui->label_IconCloudHub->setVisible(false);
    ui->label_IconDingTalk->setVisible(false);
    this->hide();
}

///同步icon
void TopBar::SyncIcon(int appId, bool hasMsg, bool hasIcon)
{
    switch(appId)
    {
    case IdWeChat:
        if(hasIcon)
        {
            ui->label_IconWeChat->setPixmap(QPixmap("src/icon/WeChat_24x.png"));
        }
        else
        {
            ui->label_IconWeChat->clear();
        }
        ui->label_IconWeChat->setVisible(hasMsg);
        break;
    case IdQQ:
        if(hasIcon)
        {
            ui->label_IconQQ->setPixmap(QPixmap("src/icon/QQ_24x.png"));
        }
        else
        {
            ui->label_IconQQ->clear();
        }
        ui->label_IconQQ->setVisible(hasMsg);
        break;
    case IdCloudHub:
        if(hasIcon)
        {
            ui->label_IconCloudHub->setPixmap(QPixmap("src/icon/CloudHub_24x.png"));
        }
        else
        {
            ui->label_IconCloudHub->clear();
        }
        ui->label_IconCloudHub->setVisible(hasMsg);
        break;
    case IdDingTalk:
        if(hasIcon)
        {
            ui->label_IconDingTalk->setPixmap(QPixmap("src/icon/DingTalk_24x.png"));
        }
        else
        {
            ui->label_IconDingTalk->clear();
        }
        ui->label_IconDingTalk->setVisible(hasMsg);
        break;
    default:
        break;
    }
}
