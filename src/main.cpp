#include <QApplication>

#include "app/MainWindow.h"
#include "common/Util.h"

#include <spdlog/spdlog.h>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 1) spdlog
    util::initializeLogging(spdlog::level::debug);

    // 2) dcmtk dictionary
    util::initializeDcmtkDictionary();

    // 3) qss
    util::applyGlobalStyleSheet(app);

    // 4) ui
    MainWindow window;
    window.show();

    // 5) event loop
    return app.exec();
}
