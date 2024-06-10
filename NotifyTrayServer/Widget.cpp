#include "Widget.h"
#include "ui_Widget.h"

#include <QTimer>
#include <QDebug>

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
    delete ui;
}

///初始化
void Widget::haku()
{
    mDwMessage.insert(NIM_ADD,          "NIM_ADD");
    mDwMessage.insert(NIM_MODIFY,       "NIM_MODIFY");
    mDwMessage.insert(NIM_DELETE,       "NIM_DELETE");
    mDwMessage.insert(NIM_SETFOCUS,     "NIM_SETFOCUS");
    mDwMessage.insert(NIM_SETVERSION,   "NIM_SETVERSION");

    qDebug() << "HOOK result: " << installHook_NotifyWndProc((HWND)this->winId());
}

///安装 Win32 钩子
bool Widget::installHook_NotifyWndProc(HWND hWnd)
{
    bool mCallWndProcHooked = false;
    HWND hShellWnd = ::FindWindow(TEXT("Shell_TrayWnd"), nullptr);
    if(!hShellWnd || !::IsWindow(hShellWnd))
    {
        qDebug() << "Not found Shell_TrayWnd";
        return false;
    }

    mCallWndProcHooked = InstallCallWndProcHook(hWnd, hShellWnd);
    if(mCallWndProcHooked)
    {
        qDebug() << "Hook CallWndProc OK!";
        return true;
    }
    else
    {
        qDebug() << "Hook CallWndProc FAILED";
        return false;
    }
}

///卸载 Win32 钩子
void Widget::uninstallHook_NotifyWndProc()
{
    UninstallCallWndProcHook();
    qDebug() << "Unhook CallWndProc";
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
        COPYDATASTRUCT* lpReceiveCDS = reinterpret_cast<COPYDATASTRUCT*>(pMsg->lParam);
        PTRAY_ICON_DATAW lpNotifyData = nullptr;
        if(lpReceiveCDS == nullptr || IsBadReadPtr(lpReceiveCDS, sizeof(TRAY_ICON_DATAW)) == TRUE)
        {
            break;
        }
        qDebug() << "lpReceiveCDS->cbData: " << lpReceiveCDS->cbData << "sizeof(TRAY_ICON_DATAW): " << sizeof(TRAY_ICON_DATAW);

        //测试时，只实现了 WM_NotifyCallWndProc 钩子的消息处理
        if(lpReceiveCDS->dwData == WM_NotifyCallWndProc && lpReceiveCDS->cbData == sizeof(TRAY_ICON_DATAW))
        {
            lpNotifyData = (TRAY_ICON_DATAW*)lpReceiveCDS->lpData;

            //输出结果
            qDebug() << QString("CTray-NotifyIconMsg:[%1]:[HWND:0x%2]").arg(mDwMessage[lpNotifyData->dwMessage]).arg(QString::number(pMsg->wParam, 16));
            qDebug() << "icon_id: " << lpNotifyData->uIconID << " title: " << QString::fromWCharArray(lpNotifyData->szTip);

            if((lpNotifyData->uFlags & NIF_INFO) != 0)
            {
                qDebug() << QString("Tip[%1], szInfoParam:").arg(QString::fromWCharArray(lpNotifyData->szTip));
                qDebug() << QString("InfoTitle[%1], Info[%2], InfoFlags[%3]")
                            .arg(QString::fromWCharArray(lpNotifyData->szInfoTitle),
                                 QString::fromWCharArray(lpNotifyData->szInfo),
                                 QString::number(lpNotifyData->dwInfoFlags));
            }
            else if((lpNotifyData->uFlags & NIF_TIP) != 0)
            {
                qDebug() << QString("Tip[%1], non-szInfo").arg(QString::fromWCharArray(lpNotifyData->szTip));
            }
            else
            {
                qDebug() << QString("non-szTip, non-szInfo");
            }
        }
    }
        break;
    default:
        break;
    }

    return QWidget::nativeEvent(eventType, message, result);
}

