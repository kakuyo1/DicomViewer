#pragma once

#include <QListView>

class ThumbnailPanel : public QListView
{
    Q_OBJECT

public:
    explicit ThumbnailPanel(QWidget *parent = nullptr);
    ~ThumbnailPanel();

private:
    void setupUi();
};
