#pragma once

#include <QWidget>

class StackPage : public QWidget
{
    Q_OBJECT

public:
    explicit StackPage(QWidget *parent = nullptr);
    ~StackPage();

private:
    void setupUi();
};
