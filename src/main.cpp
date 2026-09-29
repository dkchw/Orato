#include <QApplication>
#include <QIcon>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("Orato");
    app.setApplicationDisplayName("Orato — Speech & Pronunciation Studio");
    app.setOrganizationName("OratoStudio");
    app.setOrganizationDomain("orato.local");

    MainWindow window;
    window.show();

    return app.exec();
}
