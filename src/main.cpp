#include <QApplication>
#include <QIcon>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("Orato");
    app.setApplicationDisplayName("Orato — Speech & Pronunciation Studio");
    app.setOrganizationName("OratoStudio");
    app.setOrganizationDomain("orato.local");

    QIcon appIcon;
    appIcon.addFile(":/icons/orato-16.png", QSize(16, 16));
    appIcon.addFile(":/icons/orato-32.png", QSize(32, 32));
    appIcon.addFile(":/icons/orato-48.png", QSize(48, 48));
    appIcon.addFile(":/icons/orato-64.png", QSize(64, 64));
    appIcon.addFile(":/icons/orato-128.png", QSize(128, 128));
    appIcon.addFile(":/icons/orato-256.png", QSize(256, 256));
    appIcon.addFile(":/icons/orato-512.png", QSize(512, 512));
    appIcon.addFile(":/icons/orato.png");
    app.setWindowIcon(appIcon);

    MainWindow window;
    window.setWindowIcon(appIcon);
    window.show();

    return app.exec();
}
