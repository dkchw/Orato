#include <QApplication>
#include <QIcon>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("Recorder");
    app.setApplicationDisplayName("Recorder — Speech & Pronunciation Studio");
    app.setOrganizationName("SpeechStudio");
    app.setOrganizationDomain("speechstudio.local");

    MainWindow window;
    window.show();

    return app.exec();
}
