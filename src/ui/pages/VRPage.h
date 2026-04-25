#pragma once

#include <QWidget>

class QLabel;
class ViewerSession;

class VRPage : public QWidget
{
    Q_OBJECT

public:
    explicit VRPage(QWidget *parent = nullptr);
    ~VRPage();

    void setViewerSession(ViewerSession *viewerSession);
    void resetView();

private:
    void setupUi();
    void refreshFromSession();
    void clearDisplay();

private:
    ViewerSession *mViewerSession = nullptr;
    QLabel *mTitleLabel           = nullptr;
    QLabel *mHintLabel            = nullptr;
};
