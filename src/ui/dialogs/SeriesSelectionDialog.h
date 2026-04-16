#pragma once

#include <QDialog>

#include <QVector>

#include "core/model/dicom/DicomSeries.h"

class QListWidget;
class QPushButton;

class SeriesSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SeriesSelectionDialog(const QVector<DicomSeries> &seriesList, QWidget *parent = nullptr);

    int selectedSeriesIndex() const;

private:
    void setupUi();
    void populateSeriesList();
    void updateAcceptButtonState();

private:
    QVector<DicomSeries> mSeriesList;
    QListWidget *mSeriesListWidget = nullptr;
    QPushButton *mOpenButton       = nullptr;
};
