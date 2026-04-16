#include "common/Util.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QStringList>

#include <string>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace
{

QString resolveProjectRelativePath(const QString &relativePath)
{
    const QFileInfo inputInfo(relativePath);
    if (inputInfo.isAbsolute() && inputInfo.exists()) {
        return inputInfo.absoluteFilePath();
    }

    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QFileInfo(relativePath).absoluteFilePath(),
        QDir::current().absoluteFilePath(relativePath),
        QDir(appDir).absoluteFilePath(relativePath),
        QDir(appDir).absoluteFilePath(QStringLiteral("../") + relativePath),
        QDir(appDir).absoluteFilePath(QStringLiteral("../../") + relativePath),
        QDir(appDir).absoluteFilePath(QStringLiteral("../../../") + relativePath),
    };

    for (const QString &candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return QFileInfo(candidate).absoluteFilePath();
        }
    }

    return {};
}

} // namespace

namespace util
{

void initializeLogging(spdlog::level::level_enum level)
{
    constexpr auto kLoggerName = "dicomviewer";

    auto logger = spdlog::get(kLoggerName);
    if (!logger) {
        logger = spdlog::stdout_color_mt(std::string(kLoggerName));
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        spdlog::set_default_logger(logger);
    }

    logger->set_level(level);
    spdlog::set_level(level);
    spdlog::flush_on(spdlog::level::warn);

    spdlog::info("spdlog initialized");
}

bool initializeDcmtkDictionary()
{
    const QString dictionaryPath = resolveProjectRelativePath(
        QStringLiteral("third_party/dcmtk/share/dcmtk-3.7.0-DEV/dicom.dic"));
    if (dictionaryPath.isEmpty()) {
        spdlog::warn("Failed to locate DCMTK dicom.dic");
        return false;
    }

    if (!qputenv("DCMDICTPATH", dictionaryPath.toUtf8())) {
        spdlog::warn("Failed to set DCMDICTPATH");
        return false;
    }

    spdlog::info("Configured DCMTK dictionary: {}", dictionaryPath.toStdString());
    return true;
}

bool applyGlobalStyleSheet(QApplication &app)
{
    return applyGlobalStyleSheet(app, QStringLiteral("src/theme/style.qss"));
}

bool applyGlobalStyleSheet(QApplication &app, const QString &styleSheetPath)
{
    const QString resolvedPath = resolveProjectRelativePath(styleSheetPath);
    if (resolvedPath.isEmpty()) {
        spdlog::warn("Failed to locate style sheet: {}", styleSheetPath.toStdString());
        return false;
    }

    QFile styleFile(resolvedPath);
    if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        spdlog::warn("Failed to open style sheet: {}", resolvedPath.toStdString());
        return false;
    }

    const QString styleSheet = QString::fromUtf8(styleFile.readAll());
    app.setStyleSheet(styleSheet);

    spdlog::info("Applied global style sheet: {}", resolvedPath.toStdString());
    return true;
}

} // namespace util
