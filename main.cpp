#include <QApplication>
#include <QMessageBox>
#include <stdio.h>
#include <stdlib.h>

#include "window.h"


void myMessageOutput(QtMsgType type, const char *msg)
{
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s\n", msg);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s\n", msg);
        QMessageBox::warning(0,"Tongue Viewer Warning",QString(msg));
        break;
    case QtCriticalMsg:
        QMessageBox::critical(0,"Tongue Viewer Critical Message",QString(msg));
        break;
    case QtFatalMsg:
        QMessageBox::critical(0,"Tongue Viewer Fatal Message",QString(msg));
        abort();
    }
}

int main(int argc, char *argv[])
{
    //    qInstallMsgHandler(myMessageOutput);
    QApplication app(argc, argv);
    Window window;
    if(!window.unrecoverable)
    {
        window.show();
        return app.exec();
    }
    else
    {
        return 1;
    }
}
