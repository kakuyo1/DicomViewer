#pragma once

#include <QString>

#include <spdlog/common.h>

class QApplication;

namespace util
{

QString resolveProjectRelativePath(const QString &relativePath);

void initializeLogging(spdlog::level::level_enum level = spdlog::level::debug);

/**
 *  @note 如果没有正确设置字典，DCMTK 将无法将数值标签（如 (0010,0010)）
 *  转换为可读的名称（如 “Patient's Name”），
 *  也无法正确解析私有标签或非标准 DICOM 文件。
 */
bool initializeDcmtkDictionary();

bool applyGlobalStyleSheet(QApplication &app);

bool applyGlobalStyleSheet(QApplication &app, const QString &styleSheetPath);

} // namespace util
