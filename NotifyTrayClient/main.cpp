#include "Widget.h"

#include <QApplication>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QDir::setCurrent(QCoreApplication::applicationDirPath());   //设置工作目录为程序所在目录
    a.setWindowIcon(QIcon(":/Res/icon/Asuna_256x.png"));
    Widget w;
    w.show();
    return a.exec();
}
