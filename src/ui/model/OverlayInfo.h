#pragma once

#include <QStringList>

struct OverlayInfo
{
    QStringList topLeftLines;
    QStringList topRightLines;
    QStringList bottomLeftLines;
    QStringList bottomRightLines;

    bool isEmpty() const
    {
        return topLeftLines.isEmpty()
            && topRightLines.isEmpty()
            && bottomLeftLines.isEmpty()
            && bottomRightLines.isEmpty();
    }
};
