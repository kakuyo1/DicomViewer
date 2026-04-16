#include "ui/dialogs/SeriesSelectionDialog.h"

#include <QDialogButtonBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

SeriesSelectionDialog::SeriesSelectionDialog(const QVector<DicomSeries> &seriesList, QWidget *parent)
    : QDialog(parent)
    , mSeriesList(seriesList)
{
    setupUi();
    populateSeriesList();
    updateAcceptButtonState();
}

int SeriesSelectionDialog::selectedSeriesIndex() const
{
    if (mSeriesListWidget == nullptr || mSeriesListWidget->currentRow() < 0) {
        return -1;
    }

    return mSeriesListWidget->currentRow();
}

void SeriesSelectionDialog::setupUi()
{
    setWindowTitle(QStringLiteral("Select Series"));
    resize(640, 420);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    mSeriesListWidget = new QListWidget(this);
    mSeriesListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    mSeriesListWidget->setAlternatingRowColors(true);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    mOpenButton = buttonBox->addButton(QStringLiteral("Open"), QDialogButtonBox::AcceptRole);

    layout->addWidget(mSeriesListWidget);
    layout->addWidget(buttonBox);

    connect(mSeriesListWidget, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *) {
        if (selectedSeriesIndex() >= 0) {
            accept();
        }
    });
    connect(mSeriesListWidget, &QListWidget::currentRowChanged, this, [this](int) {
        updateAcceptButtonState();
    });
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void SeriesSelectionDialog::populateSeriesList()
{
    for (const DicomSeries &series : std::as_const(mSeriesList)) {
        const QString description = series.seriesDescription.isEmpty()
            ? QStringLiteral("(No Series Description)")
            : series.seriesDescription;
        const QString itemText = QStringLiteral(
            "%1\nModality: %2    Slices: %3\nPath: %4")
                                     .arg(description, series.modality)
                                     .arg(series.slices.size())
                                     .arg(series.pathSummary);

        auto *item = new QListWidgetItem(itemText, mSeriesListWidget);
        item->setToolTip(series.pathSummary);
    }

    if (mSeriesListWidget->count() > 0) {
        mSeriesListWidget->setCurrentRow(0);
    }
}

void SeriesSelectionDialog::updateAcceptButtonState()
{
    if (mOpenButton != nullptr) {
        mOpenButton->setEnabled(selectedSeriesIndex() >= 0);
    }
}
