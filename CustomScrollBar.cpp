#include "CustomScrollBar.hpp"
#include <QEvent>

CustomScrollBar::CustomScrollBar(Qt::Orientation orientation, QWidget* parent)
    : QScrollBar(orientation, parent)
{
}

void CustomScrollBar::wheelEvent(QWheelEvent* event)
{
    // Полностью блокируем события колеса мыши
    // Это предотвращает конфликт с изменением gain
    event->accept(); // Принимаем событие, но не обрабатываем
}

bool CustomScrollBar::event(QEvent* event)
{
    // Дополнительно блокируем события колеса мыши на уровне event
    if (event->type() == QEvent::Wheel) {
        event->accept();
        return true; // Событие обработано, не передаем дальше
    }
    
    // Для всех остальных событий используем стандартную обработку
    return QScrollBar::event(event);
}
