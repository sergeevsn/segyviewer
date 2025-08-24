#pragma once

#include <QColor>
#include <QString>
#include <QStringList>

class ColorSchemes {
public:
    // Получить цвет по нормализованному значению [0..1] и имени схемы
    static QColor getColor(float normalizedValue, const QString& schemeName);

    // Получить список доступных схем
    static QStringList getAvailableSchemes();

    // Проверить, существует ли схема
    static bool hasScheme(const QString& schemeName);

private:
    // Интерполяция между двумя цветами
    static QColor interpolateColor(const QColor& c1, const QColor& c2, float t);

    // Отдельные схемы
    static QColor grayscale(float v);
    static QColor seismic(float v);
    static QColor puOr(float v);
};
