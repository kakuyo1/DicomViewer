#pragma once

#include <QStackedWidget>

#include "../pages/StackPage.h"

class WorkSpaceWidget : public QStackedWidget
{
    Q_OBJECT

public:
    explicit WorkSpaceWidget(QWidget *parent = nullptr);
    ~WorkSpaceWidget();

private:
    void setupUi();

private:
    StackPage *mStackPage = nullptr;
};
