#pragma once

#include <QColor>
#include <QString>
#include <vector>
#include <map>

class ColorSchemes {
public:
    // Получить цвет по амплитуде и схеме
    static QColor getColor(float normalizedValue, const QString& schemeName);
    
    // Получить список доступных схем
    static QStringList getAvailableSchemes();
    
    // Проверить, существует ли схема
    static bool hasScheme(const QString& schemeName);

private:
    // Нормализация значения в диапазон [0, 1]
    static float normalizeValue(float value);
    
    // Цветовые схемы
    static QColor grayScheme(float value);
    static QColor puOrScheme(float value);
    static QColor seismicScheme(float value);
    
    // Вспомогательные функции для интерполяции цветов
    static QColor interpolateColor(const QColor& color1, const QColor& color2, float t);
    static QColor interpolateColor(const std::vector<QColor>& colors, float value);
    
    // Определения цветовых палитр
    static std::vector<QColor> getGrayPalette();
    static std::vector<QColor> getPuOrPalette();
    static std::vector<QColor> getSeismicPalette();
};
