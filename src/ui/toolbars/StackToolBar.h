#pragma once

#include <QWidget>

class StackToolBar : public QWidget
{
    Q_OBJECT

public:
    explicit StackToolBar(QWidget *parent = nullptr);
    ~StackToolBar();

private:
    void setupUi();
};
