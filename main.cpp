#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setApplicationName("scraduino");
    QApplication::setOrganizationName("scraduino");

    MainWindow w;
    w.show();
    
    return a.exec();
}
