#pragma once

#include <QString>

#include <spdlog/common.h>

class QApplication;

namespace util
{

void initializeLogging(spdlog::level::level_enum level = spdlog::level::debug);

bool applyGlobalStyleSheet(QApplication &app);

bool applyGlobalStyleSheet(QApplication &app, const QString &styleSheetPath);

} // namespace util
