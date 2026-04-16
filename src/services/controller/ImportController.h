#pragma once

#include <optional>

#include <QString>

#include "services/model/ImportResult.h"

class QWidget;

class ImportController
{
public:
    std::optional<ImportResult> importFromFolder(QWidget *dialogParent, QString *errorMessage = nullptr) const;
};
