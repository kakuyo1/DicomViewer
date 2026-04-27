#pragma once

#include <QString>

struct DicomVector3
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

/**
 * @brief 关于 IPP/IOP 的解释
 * 1.拿到了 IOP，我就能知道当体素在 X 方向增加时，对应的病人方向是 L 还是 R——这取决于
 *   volumeXDirection 的绝对值最大分量：如果 X 分量最大且为正则是 L，为负则是 R。
 * 2.同理，当体素在 Y 方向增加时，能判断病人方向是 P 还是 A——这取决于 volumeYDirection
 *   的绝对值最大分量：如果 Y 分量最大且为正则是 P，为负则是 A。
 * 3.拿到了 IPP（根据第一张和最后一张 slice 的 IPP 得到一个向量），配合通过 IOP 的 row
 *   和 column 叉积计算出的 normal 进行点乘，来判断 volumeZDirection 应该是 +normal 还是 -normal：
 *      - 点乘结果 ≥ 0，说明 slices 的排列方向和 normal 一致，volumeZDirection = normal
 *      - 点乘结果 < 0，说明 slices 的排列方向和 normal 相反，volumeZDirection = -normal
 *   最后，用和 X/Y 同样的方法，取 volumeZDirection 绝对值最大的分量来判断病人方向：Z 分量最大且为正则是 S，为负则是 I。
 */
struct DicomSliceInfo
{
    QString filePath;

    QString patientName;
    QString patientId;
    QString patientSex; // M/F/O
    QString patientAge;
    QString patientBirthDate;

    QString studyDate; // 检查开始的日期
    QString studyTime; // 检查开始的时间

    QString seriesInstanceUid; // 序列实例 UID: 全球唯一（跨医院、跨设备、跨时间）
    QString sopInstanceUid;    // SOP 实例 UID（单个图像的全局唯一标识）: 全球唯一
    QString seriesDescription;
    QString modality; // CT/MR/...

    int  seriesNumber    = 0; // 序列号: 单个 Study 内部唯一
    bool hasSeriesNumber = false;

    int  instanceNumber    = 0; // 实例号（图像在序列中的编号）: 单个 Series 内部唯一
    bool hasInstanceNumber = false;

    DicomVector3 imagePositionPatient;
    DicomVector3 rowDirection;
    DicomVector3 columnDirection;
    DicomVector3 sliceNormal; /** @note sliceNormal = rowDirection [cross] columnDirection */
    bool         hasImagePositionPatient    = false;
    bool         hasImageOrientationPatient = false;
    bool         hasSliceNormal             = false;

    double sliceThickness    = 0.0;
    bool   hasSliceThickness = false;

    double sliceLocation    = 0.0;
    bool   hasSliceLocation = false;

    double kvp    = 0.0; // 峰值管电压: 决定 X 射线的穿透能力（对比度）
    bool   hasKvp = false;

    double tubeCurrentMa    = 0.0; // 管电流: 决定 X 射线的强度（即光子数量，与图像噪声直接相关）
    bool   hasTubeCurrentMa = false;

    double windowCenter    = 0.0;
    double windowWidth     = 0.0;
    bool   hasWindowCenter = false;
    bool   hasWindowWidth  = false;
};

struct DicomReadResult
{
    bool           success                  = false;
    bool           readableDicom            = false;
    bool           missingSeriesInstanceUid = false;
    DicomSliceInfo sliceInfo;
    QString        errorMessage;
};
