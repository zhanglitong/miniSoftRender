
#include    <QtWidgets/QApplication>
#include    <QMessageBox>
#include    <string>
#include    <map>
#include    "MainWindow.h"


int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL, true);
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings, true);
#if (QT_VERSION >=QT_VERSION_CHECK(6,0,0))
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Floor);
#endif

    QApplication    a(argc, argv);
    MainWindow      win;
    win.show();
    
    return  a.exec();
}
