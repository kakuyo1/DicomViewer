#pragma once

#include <QWidget>

class ViewModeBar : public QWidget
{
    Q_OBJECT

public:
    explicit ViewModeBar(QWidget *parent = nullptr);
    ~ViewModeBar();

private:
    void setupUi();
};
