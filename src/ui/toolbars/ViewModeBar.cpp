#include "ViewModeBar.h"

#include <QButtonGroup>
#include <QIcon>
#include <QToolButton>
#include <QVBoxLayout>

#include "common/Util.h"

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
    setObjectName(QStringLiteral("viewModeBar"));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    setFixedWidth(180);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(14);

    mModeGroup = new QButtonGroup(this);
    mModeGroup->setExclusive(true);

    mStackButton = createModeButton(QStringLiteral("Stack View"), QStringLiteral("resources/icons/StackView.png"));
    mMprButton   = createModeButton(QStringLiteral("MPR View"),   QStringLiteral("resources/icons/MPRView.png"));
    mVrButton    = createModeButton(QStringLiteral("VR View"),    QStringLiteral("resources/icons/VRView.png"));

    mModeGroup->addButton(mStackButton);
    mModeGroup->addButton(mMprButton);
    mModeGroup->addButton(mVrButton);

    mStackButton->setChecked(true);

    connect(mStackButton, &QToolButton::clicked, this, [this]() {
        emit viewModeChanged(QStringLiteral("Stack View"));
    });
    connect(mMprButton, &QToolButton::clicked, this, [this]() {
        emit viewModeChanged(QStringLiteral("MPR View"));
    });
    connect(mVrButton, &QToolButton::clicked, this, [this]() {
        emit viewModeChanged(QStringLiteral("VR View"));
    });

    layout->addWidget(mStackButton, 0, Qt::AlignHCenter);
    layout->addWidget(mMprButton,   0, Qt::AlignHCenter);
    layout->addWidget(mVrButton,    0, Qt::AlignHCenter);
}

QToolButton *ViewModeBar::createModeButton(const QString &text, const QString &iconPath)
{
    auto *button = new QToolButton(this);
    button->setText(text);
    button->setCheckable(true);
    button->setFixedSize(132, 104);
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    button->setIconSize(QSize(58, 58));
    button->setAutoRaise(false);
    button->setCursor(Qt::PointingHandCursor);

    const QString resolvedIconPath = util::resolveProjectRelativePath(iconPath);
    if (!resolvedIconPath.isEmpty()) {
        button->setIcon(QIcon(resolvedIconPath));
    }

    return button;
}
