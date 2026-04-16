#include "StackPage.h"

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>

StackPage::StackPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

StackPage::~StackPage()
{

}

void StackPage::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto *placeholderFrame = new QFrame(this);
    placeholderFrame->setFrameShape(QFrame::Box);
    placeholderFrame->setMinimumSize(640, 480);

    auto *frameLayout = new QVBoxLayout(placeholderFrame);
    frameLayout->setContentsMargins(16, 16, 16, 16);

    auto *placeholderLabel = new QLabel(QStringLiteral("StackPage"), placeholderFrame);
    placeholderLabel->setAlignment(Qt::AlignCenter);

    frameLayout->addStretch();
    frameLayout->addWidget(placeholderLabel);
    frameLayout->addStretch();

    rootLayout->addWidget(placeholderFrame);
}
