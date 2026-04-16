#include "ViewModeBar.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QStringList>

ViewModeBar::ViewModeBar(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

ViewModeBar::~ViewModeBar()
{

}

void ViewModeBar::setupUi()
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    setFixedWidth(180);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(14);

    const QStringList buttonTexts = {
        QStringLiteral("Stack View"),
        QStringLiteral("MPR View"),
        QStringLiteral("VR View")
    };

    for (const QString &text : buttonTexts) {
        auto *button = new QPushButton(text, this);
        button->setEnabled(false);
        button->setFixedSize(132, 96);
        layout->addWidget(button, 0, Qt::AlignHCenter);
    }
}
