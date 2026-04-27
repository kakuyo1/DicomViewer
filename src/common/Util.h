#pragma once

#include <QString>

#include <spdlog/common.h>

#include "core/model/volume/SliceOrientation.h"

class QApplication;
struct DicomSliceInfo;
struct DicomVector3;
struct VolumeData;

namespace util
{

struct SlicePlaneGeometry
{
    int width       = 0;
    int height      = 0;
    double spacingX = 1.0;
    double spacingY = 1.0;
};

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

QString formatDicomDate(const QString &rawDate);

QString formatDicomTime(const QString &rawTime);

QString buildSexAgeText(const DicomSliceInfo &sliceInfo);

QString formatSlicePositionText(const DicomSliceInfo &sliceInfo);

int sliceCountForOrientation(const VolumeData &volumeData, SliceOrientation orientation);

SlicePlaneGeometry sliceGeometryForOrientation(const VolumeData &volumeData, SliceOrientation orientation);

QString orientationText(SliceOrientation orientation);

QString patientDirectionLabel(const DicomVector3 &direction);

DicomVector3 reversedDirection(const DicomVector3 &direction);
} // namespace util
