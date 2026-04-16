#include "StackToolBar.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QStringList>

StackToolBar::StackToolBar(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

StackToolBar::~StackToolBar()
{

}

void StackToolBar::setupUi()
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setMinimumHeight(52);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(8);

    const QStringList buttonTexts = {
        QStringLiteral("Pan"),
        QStringLiteral("Zoom"),
        QStringLiteral("Window/Level"),
        QStringLiteral("Measure"),
        QStringLiteral("Flip H"),
        QStringLiteral("Flip V"),
        QStringLiteral("Invert")
    };

    for (const QString &text : buttonTexts) {
        auto *button = new QPushButton(text, this);
        button->setEnabled(false);
        button->setMinimumHeight(32);
        layout->addWidget(button);
    }
}
