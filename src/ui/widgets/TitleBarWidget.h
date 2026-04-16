#pragma once

#include <QPoint>
#include <QWidget>

class QLabel;
class QToolButton;

class TitleBarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBarWidget(QWidget *parent = nullptr);

    int  barHeight() const;
    void setBarHeight(int height);
    void syncWindowState();

signals:
    void openFolderRequested();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void setupUi();
    void attachToWindow();
    bool isDragArea(const QPoint &pos) const;
    void toggleMaximized();
    void updateWindowControlIcons();

private:
    QWidget     *mObservedWindow = nullptr;
    QLabel      *mLogoLabel      = nullptr;
    QToolButton *mFileButton     = nullptr;
    QToolButton *mMinimizeButton = nullptr;
    QToolButton *mMaximizeButton = nullptr;
    QToolButton *mCloseButton    = nullptr;

    bool   mDragging             = false;
    QPoint mDragOffset;
    int mBarHeight               = 40;
};
