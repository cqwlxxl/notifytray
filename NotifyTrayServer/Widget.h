#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMap>
#include <QWidget>

#include <windows.h>
#include "../LibTrayHook/LibTrayHook/trayhook.h"

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

private:
    void haku();                                        //初始化
    bool installHook_NotifyWndProc(HWND hWnd);          //安装 Win32 钩子
    void uninstallHook_NotifyWndProc();                 //卸载 Win32 钩子

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, long *result);

private:
    QMap<int, QString>      mDwMessage;
};
#endif // WIDGET_H
