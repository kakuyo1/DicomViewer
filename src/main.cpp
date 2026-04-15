#include <QApplication>

#include "app/MainWindow.h"
#include "common/Util.h"

#include <spdlog/spdlog.h>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 1) spdlog
    util::initializeLogging(spdlog::level::debug);

    // 2) qss
    util::applyGlobalStyleSheet(app);

    // 3) ui
    MainWindow window;
    window.show();

    // 4) event loop
    return app.exec();
}
