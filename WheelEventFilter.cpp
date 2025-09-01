#include "WheelEventFilter.hpp"
#include <QScrollBar>
#include <QDebug>

WheelEventFilter::WheelEventFilter(QObject* parent)
    : QObject(parent)
{
}

bool WheelEventFilter::eventFilter(QObject* obj, QEvent* event)
{
    // Блокируем события колеса мыши только для скролл баров
    if (event->type() == QEvent::Wheel) {
        if (qobject_cast<QScrollBar*>(obj)) {
            event->accept();
            return true; // Событие обработано, не передаем дальше
        }
    }
    
    // Для всех остальных объектов передаем событие дальше
    return QObject::eventFilter(obj, event);
}
