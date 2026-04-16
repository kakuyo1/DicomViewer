#include "ThumbnailPanel.h"

#include <QStringListModel>

ThumbnailPanel::ThumbnailPanel(QWidget *parent)
    : QListView(parent)
{
    setupUi();
}

ThumbnailPanel::~ThumbnailPanel()
{

}

void ThumbnailPanel::setupUi()
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setFixedWidth(220);

    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setUniformItemSizes(true);
    setSpacing(8);

    auto *model = new QStringListModel(this);
    model->setStringList({
        QStringLiteral("Slice 001"),
        QStringLiteral("Slice 002"),
        QStringLiteral("Slice 003"),
        QStringLiteral("Slice 004"),
        QStringLiteral("Slice 005")
    });
    setModel(model);
}
