#pragma once

#include <QWidget>
#include <QPushButton>

class StackToolBar : public QWidget
{
    Q_OBJECT
public:
    explicit StackToolBar(QWidget* parent = nullptr);
    ~StackToolBar();
private:
};
