#pragma once
#include <QWidget>

class QLabel;

class StatusPanel : public QWidget {
    Q_OBJECT
public:
    explicit StatusPanel(QWidget *parent = nullptr);
    void updateInfo(int traceIndex, int sampleIndex, float amplitude, float dt);

private:
    QLabel* label;
};

