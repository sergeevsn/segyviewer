#pragma once

#include <QObject>
#include <QEvent>
#include <QWheelEvent>

class WheelEventFilter : public QObject {
    Q_OBJECT

public:
    explicit WheelEventFilter(QObject* parent = nullptr);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
};
