#include "ColorSchemes.hpp"
#include <algorithm>
#include <vector>

namespace {
    inline float normalizeValue(float v) {
        if (v < 0.0f) return 0.0f;
        if (v > 1.0f) return 1.0f;
        return v;
    }

    inline int clampInt(int v, int lo, int hi) {
        return std::max(lo, std::min(v, hi));
    }
}

QColor ColorSchemes::interpolateColor(const QColor& c1, const QColor& c2, float t) {
    t = normalizeValue(t);

    int r = static_cast<int>(c1.red()   * (1.0f - t) + c2.red()   * t + 0.5f);
    int g = static_cast<int>(c1.green() * (1.0f - t) + c2.green() * t + 0.5f);
    int b = static_cast<int>(c1.blue()  * (1.0f - t) + c2.blue()  * t + 0.5f);

    return QColor(clampInt(r, 0, 255),
                  clampInt(g, 0, 255),
                  clampInt(b, 0, 255));
}

// ------------------------ схемы ------------------------

QColor ColorSchemes::grayscale(float v) {
    v = normalizeValue(v);
    int c = static_cast<int>(v * 255.0f + 0.5f);
    return QColor(c, c, c);
}

QColor ColorSchemes::seismic(float v) {
    v = normalizeValue(v);

    if (v < 0.5f) {
        float t = v / 0.5f;
        return interpolateColor(QColor(0, 0, 255), QColor(255, 255, 255), t);
    } else {
        float t = (v - 0.5f) / 0.5f;
        return interpolateColor(QColor(255, 255, 255), QColor(255, 0, 0), t);
    }
}

QColor ColorSchemes::puOr(float v) {
    v = normalizeValue(v);

    // Простая интерполяция по 5 ключевым цветам (фиолет->белый->оранж)
    std::vector<QColor> palette = {
        QColor(127, 0, 127),    // темно-фиолетовый
        QColor(191, 95, 168),   // светлее фиолет
        QColor(255, 255, 255),  // белый центр
        QColor(255, 160, 0),    // светлый оранж
        QColor(127, 60, 0)      // темно-оранжевый
    };

    float segmentSize = 1.0f / (palette.size() - 1);
    int idx = static_cast<int>(v / segmentSize);
    if (idx >= static_cast<int>(palette.size() - 1))
        return palette.back();

    float localT = (v - idx * segmentSize) / segmentSize;
    return interpolateColor(palette[idx], palette[idx + 1], localT);
}

// ------------------------ API ------------------------

QColor ColorSchemes::getColor(float normalizedValue, const QString& schemeName) {
    if (schemeName == "gray") {
        return grayscale(normalizedValue);
    } else if (schemeName == "seismic") {
        return seismic(normalizedValue);
    } else if (schemeName == "PuOr") {
        return puOr(normalizedValue);
    } else {
        // По умолчанию gray
        return grayscale(normalizedValue);
    }
}

QStringList ColorSchemes::getAvailableSchemes() {
    return {"gray", "seismic", "PuOr"};
}

bool ColorSchemes::hasScheme(const QString& schemeName) {
    return getAvailableSchemes().contains(schemeName);
}
