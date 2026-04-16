#include "core/worker/dicom/DicomTagReader.h"

#include <dcmtk/dcmdata/dctk.h>

namespace
{

QString readStringValue(DcmDataset *dataset, const DcmTagKey &tagKey)
{
    OFString value;
    if (dataset->findAndGetOFString(tagKey, value).good()) {
        return QString::fromUtf8(value.c_str()).trimmed();
    }

    return {};
}

} // namespace

std::optional<DicomSliceInfo> DicomTagReader::readSliceInfo(const QString &filePath) const
{
    DcmFileFormat fileFormat;
    if (!fileFormat.loadFile(filePath.toUtf8().constData()).good()) {
        return std::nullopt;
    }

    DcmDataset *dataset = fileFormat.getDataset();
    if (dataset == nullptr) {
        return std::nullopt;
    }

    DicomSliceInfo sliceInfo;
    sliceInfo.filePath          = filePath;
    sliceInfo.seriesInstanceUid = readStringValue(dataset, DCM_SeriesInstanceUID);
    sliceInfo.sopInstanceUid    = readStringValue(dataset, DCM_SOPInstanceUID);
    sliceInfo.seriesDescription = readStringValue(dataset, DCM_SeriesDescription);
    sliceInfo.modality          = readStringValue(dataset, DCM_Modality).toUpper();

    if (sliceInfo.seriesInstanceUid.isEmpty()) {
        return std::nullopt;
    }

    Sint32 instanceNumber = 0;
    if (dataset->findAndGetSint32(DCM_InstanceNumber, instanceNumber).good()) {
        sliceInfo.instanceNumber = static_cast<int>(instanceNumber);
        sliceInfo.hasInstanceNumber = true;
    }

    return sliceInfo;
}
