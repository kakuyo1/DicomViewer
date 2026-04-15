#pragma once

#include <QWidget>
#include <QPushButton>

class ViewModeBar : public QWidget
{
    Q_OBJECT
public:
    explicit ViewModeBar(QWidget* parent = nullptr);
    ~ViewModeBar();
private:
};
