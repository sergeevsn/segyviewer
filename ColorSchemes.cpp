#include "ColorSchemes.hpp"
#include <QStringList>
#include <algorithm>
#include <cmath>

QColor ColorSchemes::getColor(float normalizedValue, const QString& schemeName) {
    // Нормализуем значение в диапазон [0, 1]
    float value = normalizeValue(normalizedValue);
    
    if (schemeName == "gray") {
        return grayScheme(value);
    } else if (schemeName == "PuOr") {
        return puOrScheme(value);
    } else if (schemeName == "seismic") {
        return seismicScheme(value);
    } else {
        // По умолчанию используем gray
        return grayScheme(value);
    }
}

QStringList ColorSchemes::getAvailableSchemes() {
    return {"gray", "PuOr", "seismic"};
}

bool ColorSchemes::hasScheme(const QString& schemeName) {
    QStringList schemes = getAvailableSchemes();
    return schemes.contains(schemeName);
}

float ColorSchemes::normalizeValue(float value) {
    return std::max(0.0f, std::min(1.0f, value));
}

QColor ColorSchemes::grayScheme(float value) {
    // Простая серая схема: от черного к белому
    int intensity = static_cast<int>(value * 255.0f + 0.5f);
    intensity = std::max(0, std::min(255, intensity));
    return QColor(intensity, intensity, intensity);
}

QColor ColorSchemes::puOrScheme(float value) {
    // PuOr (Purple-Orange) схема из matplotlib
    // Переворачиваем значение для правильной ориентации
    float reversedValue = 1.0f - value;
    
    std::vector<QColor> palette = getPuOrPalette();
    return interpolateColor(palette, reversedValue);
}

QColor ColorSchemes::seismicScheme(float value) {
    // Seismic схема из matplotlib
    // Центрируем значение вокруг 0.5
    float centeredValue = (value - 0.5f) * 2.0f; // [-1, 1]
    float absValue = std::abs(centeredValue);
    float sign = (centeredValue >= 0) ? 1.0f : -1.0f;
    
    std::vector<QColor> palette = getSeismicPalette();
    
    if (sign > 0) {
        // Красная часть (положительные значения)
        return interpolateColor(palette, absValue);
    } else {
        // Синяя часть (отрицательные значения)
        std::vector<QColor> bluePalette = {palette[0], palette[1]};
        return interpolateColor(bluePalette, absValue);
    }
}

QColor ColorSchemes::interpolateColor(const QColor& color1, const QColor& color2, float t) {
    t = normalizeValue(t);
    
    int r = static_cast<int>(color1.red() * (1.0f - t) + color2.red() * t + 0.5f);
    int g = static_cast<int>(color1.green() * (1.0f - t) + color2.green() * t + 0.5f);
    int b = static_cast<int>(color1.blue() * (1.0f - t) + color2.blue() * t + 0.5f);
    
    r = std::max(0, std::min(255, r));
    g = std::max(0, std::min(255, g));
    b = std::max(0, std::min(255, b));
    
    return QColor(r, g, b);
}

QColor ColorSchemes::interpolateColor(const std::vector<QColor>& colors, float value) {
    if (colors.empty()) return Qt::black;
    if (colors.size() == 1) return colors[0];
    
    value = normalizeValue(value);
    
    // Находим два ближайших цвета для интерполяции
    float segmentSize = 1.0f / (colors.size() - 1);
    int segmentIndex = static_cast<int>(value / segmentSize);
    
    if (segmentIndex >= static_cast<int>(colors.size() - 1)) {
        return colors.back();
    }
    
    float localValue = (value - segmentIndex * segmentSize) / segmentSize;
    return interpolateColor(colors[segmentIndex], colors[segmentIndex + 1], localValue);
}

std::vector<QColor> ColorSchemes::getGrayPalette() {
    return {
        QColor(0, 0, 0),        // Черный
        QColor(255, 255, 255)   // Белый
    };
}

std::vector<QColor> ColorSchemes::getPuOrPalette() {
    // PuOr (Purple-Orange) палитра из matplotlib
    return {
        QColor(127, 0, 127),    // Темно-фиолетовый
        QColor(196, 121, 0),    // Темно-оранжевый
        QColor(255, 255, 255),  // Белый (центр)
        QColor(255, 255, 255),  // Белый (центр)
        QColor(196, 121, 0),    // Темно-оранжевый
        QColor(127, 0, 127)     // Темно-фиолетовый
    };
}

std::vector<QColor> ColorSchemes::getSeismicPalette() {
    // Seismic палитра из matplotlib
    return {
        QColor(0, 0, 255),      // Синий (отрицательные значения)
        QColor(255, 255, 255),  // Белый (нулевые значения)
        QColor(255, 0, 0)       // Красный (положительные значения)
    };
}
