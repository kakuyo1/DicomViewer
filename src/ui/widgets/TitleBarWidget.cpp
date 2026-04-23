#include "ui/widgets/TitleBarWidget.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMouseEvent>
#include <QPixmap>
#include <QShowEvent>
#include <QStyle>
#include <QToolButton>
#include <QWindow>

#include "common/Util.h"

namespace
{

QIcon loadAppLogoIcon()
{
    const QString logoPath = util::resolveProjectRelativePath(QStringLiteral("resources/logo/Logo-V1.png"));
    if (logoPath.isEmpty()) {
        return {};
    }

    return QIcon(logoPath);
}

QPoint mouseGlobalPos(const QMouseEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->globalPosition().toPoint();
#else
    return event->globalPos();
#endif
}

} // namespace

TitleBarWidget::TitleBarWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

int TitleBarWidget::barHeight() const
{
    return mBarHeight;
}

void TitleBarWidget::setBarHeight(int height)
{
    mBarHeight = qMax(40, height);
    setFixedHeight(mBarHeight);
}

void TitleBarWidget::syncWindowState()
{
    attachToWindow();
    updateWindowControlIcons();

    if (mObservedWindow != nullptr) {
        QIcon logoIcon = mObservedWindow->windowIcon();
        if (logoIcon.isNull()) {
            logoIcon = loadAppLogoIcon();
        }
        if (logoIcon.isNull()) {
            logoIcon = style()->standardIcon(QStyle::SP_ComputerIcon);
        }

        if (mObservedWindow->windowIcon().isNull() && !logoIcon.isNull()) {
            mObservedWindow->setWindowIcon(logoIcon);
        }
        mLogoLabel->setPixmap(logoIcon.pixmap(30, 26));
    }
}

bool TitleBarWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == mObservedWindow) {
        if (event->type() == QEvent::WindowStateChange || event->type() == QEvent::WindowTitleChange) {
            syncWindowState();
        }
    }

    return QWidget::eventFilter(watched, event);
}

void TitleBarWidget::showEvent(QShowEvent *event)
{
    attachToWindow();
    syncWindowState();
    QWidget::showEvent(event);
}

void TitleBarWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isDragArea(event->pos())) {
        if (QWindow *windowHandle = window()->windowHandle()) {
            if (windowHandle->startSystemMove()) {
                event->accept();
                return;
            }
        }

        if (!window()->isMaximized()) {
            mDragging = true;
            mDragOffset = mouseGlobalPos(event) - window()->frameGeometry().topLeft();
            event->accept();
            return;
        }
    }

    QWidget::mousePressEvent(event);
}

void TitleBarWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (mDragging && (event->buttons() & Qt::LeftButton) && !window()->isMaximized()) {
        window()->move(mouseGlobalPos(event) - mDragOffset);
        event->accept();
        return;
    }

    QWidget::mouseMoveEvent(event);
}

void TitleBarWidget::mouseReleaseEvent(QMouseEvent *event)
{
    mDragging = false;
    QWidget::mouseReleaseEvent(event);
}

void TitleBarWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isDragArea(event->pos())) {
        toggleMaximized();
        event->accept();
        return;
    }

    QWidget::mouseDoubleClickEvent(event);
}

void TitleBarWidget::setupUi()
{
    setObjectName(QStringLiteral("titleBarWidget"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(mBarHeight);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(0);

    mLogoLabel = new QLabel(this);
    mLogoLabel->setObjectName(QStringLiteral("titleBarLogoLabel"));
    mLogoLabel->setFixedSize(30, 26);
    mLogoLabel->setAlignment(Qt::AlignCenter);

    QIcon logoIcon = loadAppLogoIcon();
    if (logoIcon.isNull()) {
        logoIcon = style()->standardIcon(QStyle::SP_ComputerIcon);
    }
    mLogoLabel->setPixmap(logoIcon.pixmap(30, 26));

    mFileButton = new QToolButton(this);
    mFileButton->setObjectName(QStringLiteral("titleBarFileButton"));
    mFileButton->setAutoRaise(true);
    mFileButton->setText(QStringLiteral("File"));
    mFileButton->setCursor(Qt::PointingHandCursor);
    mFileButton->setFixedHeight(34);

    mMinimizeButton = new QToolButton(this);
    mMinimizeButton->setObjectName(QStringLiteral("titleBarMinimizeButton"));
    mMinimizeButton->setAutoRaise(true);
    mMinimizeButton->setCursor(Qt::PointingHandCursor);
    mMinimizeButton->setFixedSize(38, 30);

    mMaximizeButton = new QToolButton(this);
    mMaximizeButton->setObjectName(QStringLiteral("titleBarMaximizeButton"));
    mMaximizeButton->setAutoRaise(true);
    mMaximizeButton->setCursor(Qt::PointingHandCursor);
    mMaximizeButton->setFixedSize(38, 30);

    mCloseButton = new QToolButton(this);
    mCloseButton->setObjectName(QStringLiteral("titleBarCloseButton"));
    mCloseButton->setAutoRaise(true);
    mCloseButton->setCursor(Qt::PointingHandCursor);
    mCloseButton->setFixedSize(38, 30);

    layout->addWidget(mLogoLabel);
    layout->addWidget(mFileButton,     0, Qt::AlignVCenter);
    layout->addStretch(1);
    layout->addWidget(mMinimizeButton, 0, Qt::AlignVCenter);
    layout->addWidget(mMaximizeButton, 0, Qt::AlignVCenter);
    layout->addWidget(mCloseButton,    0, Qt::AlignVCenter);

    connect(mFileButton, &QToolButton::clicked, this, &TitleBarWidget::openFolderRequested);
    connect(mMinimizeButton, &QToolButton::clicked, this, [this]() {
        window()->showMinimized();
    });
    connect(mMaximizeButton, &QToolButton::clicked, this, [this]() {
        toggleMaximized();
    });
    connect(mCloseButton, &QToolButton::clicked, this, [this]() {
        window()->close();
    });

    updateWindowControlIcons();
}

void TitleBarWidget::attachToWindow()
{
    QWidget *topLevelWindow = window();
    if (topLevelWindow == nullptr || topLevelWindow == mObservedWindow) {
        return;
    }

    if (mObservedWindow != nullptr) {
        mObservedWindow->removeEventFilter(this);
    }

    mObservedWindow = topLevelWindow;
    mObservedWindow->installEventFilter(this);
}

bool TitleBarWidget::isDragArea(const QPoint &pos) const
{
    QWidget *child = childAt(pos);
    if (child == nullptr) {
        return true;
    }

    return child != mFileButton
        && child != mMinimizeButton
        && child != mMaximizeButton
        && child != mCloseButton;
}

void TitleBarWidget::toggleMaximized()
{
    if (window()->isMaximized()) {
        window()->showNormal();
    } else {
        window()->showMaximized();
    }

    updateWindowControlIcons();
}

void TitleBarWidget::updateWindowControlIcons()
{
    const bool maximized = window() != nullptr && window()->isMaximized();

    mMinimizeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarMinButton));
    mMaximizeButton->setIcon(
        style()->standardIcon(maximized ? QStyle::SP_TitleBarNormalButton : QStyle::SP_TitleBarMaxButton));
    mCloseButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));

    mMinimizeButton->setToolTip(QStringLiteral("Minimize"));
    mMaximizeButton->setToolTip(maximized ? QStringLiteral("Restore") : QStringLiteral("Maximize"));
    mCloseButton->setToolTip(QStringLiteral("Close"));
}
