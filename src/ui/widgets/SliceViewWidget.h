#pragma once

#include <QVTKOpenGLNativeWidget.h>

class SliceViewWidget : public QVTKOpenGLNativeWidget
{
    Q_OBJECT
public:
    explicit SliceViewWidget(QWidget *parent = nullptr);
    ~SliceViewWidget();
};