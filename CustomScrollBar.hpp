#pragma once

#include <QScrollBar>
#include <QWheelEvent>

class CustomScrollBar : public QScrollBar {
    Q_OBJECT

public:
    explicit CustomScrollBar(Qt::Orientation orientation, QWidget* parent = nullptr);

protected:
    void wheelEvent(QWheelEvent* event) override;
    bool event(QEvent* event) override;
};
