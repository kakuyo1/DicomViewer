#pragma once

#include <QString>

struct DicomVector3
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct DicomSliceInfo
{
    QString filePath;

    QString patientName;
    QString patientId;
    QString patientSex;             // M/F/O
    QString patientAge;
    QString patientBirthDate;

    QString studyDate;              // 检查开始的日期
    QString studyTime;              // 检查开始的时间

    QString seriesInstanceUid;      // 序列实例 UID: 全球唯一（跨医院、跨设备、跨时间）
    QString sopInstanceUid;         // SOP 实例 UID（单个图像的全局唯一标识）: 全球唯一
    QString seriesDescription;
    QString modality;               // CT/MR/...

    int seriesNumber       = 0;     // 序列号: 单个 Study 内部唯一
    bool hasSeriesNumber   = false;

    int instanceNumber     = 0;     // 实例号（图像在序列中的编号）: 单个 Series 内部唯一
    bool hasInstanceNumber = false;

    DicomVector3 imagePositionPatient;
    DicomVector3 rowDirection;
    DicomVector3 columnDirection;
    DicomVector3 sliceNormal;                   /** @note sliceNormal = rowDirection [cross] columnDirection */
    bool hasImagePositionPatient    = false;
    bool hasImageOrientationPatient = false;
    bool hasSliceNormal             = false;

    double sliceThickness  = 0.0;
    bool hasSliceThickness = false;

    double sliceLocation   = 0.0;
    bool hasSliceLocation  = false;

    double kvp             = 0.0;               // 峰值管电压: 决定 X 射线的穿透能力（对比度）
    bool hasKvp            = false;

    double tubeCurrentMa   = 0.0;               // 管电流: 决定 X 射线的强度（即光子数量，与图像噪声直接相关）
    bool hasTubeCurrentMa  = false;

    double windowCenter    = 0.0;
    double windowWidth     = 0.0;
    bool hasWindowCenter   = false;
    bool hasWindowWidth    = false;
};

struct DicomReadResult
{
    bool success                  = false;
    bool readableDicom            = false;
    bool missingSeriesInstanceUid = false;
    DicomSliceInfo sliceInfo;
    QString errorMessage;
};
