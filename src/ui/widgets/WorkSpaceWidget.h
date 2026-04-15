#pragma once

#include <QStackedWidget>

#include "../pages/StackPage.h"

class WorkSpaceWidget : public QStackedWidget
{
public:
    explicit WorkSpaceWidget(QWidget *parent = nullptr);
    ~WorkSpaceWidget();

private:
    StackPage* mStackPage;
};