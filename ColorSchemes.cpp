#include "ColorSchemes.hpp"
#include <algorithm>
#include <cmath>

// Статические переменные
float ColorSchemes::s_gamma = 1.0f;  // Линейная интерполяция без гамма-коррекции
bool ColorSchemes::s_perceptualCorrection = false;
std::map<QString, std::unique_ptr<ColorScheme>> ColorSchemes::s_customSchemes;

namespace {
    // XYZ цветовое пространство константы
    const float XYZ_WHITE_X = 95.047f;
    const float XYZ_WHITE_Y = 100.000f;
    const float XYZ_WHITE_Z = 108.883f;
    
    inline float sRGBtoLinear(float c) {
        return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
    }
    
    inline float linearTosRGB(float c) {
        return c <= 0.0031308f ? 12.92f * c : 1.055f * std::pow(c, 1.0f/2.4f) - 0.055f;
    }
    
    inline float labF(float t) {
        const float delta = 6.0f / 29.0f;
        return t > delta * delta * delta ? std::pow(t, 1.0f/3.0f) : t / (3.0f * delta * delta) + 4.0f / 29.0f;
    }
    
    inline float labFInv(float t) {
        const float delta = 6.0f / 29.0f;
        return t > delta ? t * t * t : 3.0f * delta * delta * (t - 4.0f / 29.0f);
    }
}

void ColorScheme::addStop(float pos, const QColor& color) {
    stops.emplace_back(pos, color);
    // Сортируем по позиции
    std::sort(stops.begin(), stops.end(), [](const ColorStop& a, const ColorStop& b) {
        return a.position < b.position;
    });
}

QColor ColorScheme::getColor(float value) const {
    if (stops.empty()) return QColor(0, 0, 0);
    if (stops.size() == 1) return stops[0].color;
    
    value = ColorSchemes::normalizeValue(value);
    
    // Применяем контрастность и яркость
    value = ColorSchemes::contrastAdjust(value, contrast, brightness);
    
    return ColorSchemes::interpolateFromPalette(stops, value);
}

// Улучшенная интерполяция в LAB пространстве
void ColorSchemes::rgbToLab(float r, float g, float b, float& L, float& a, float& lab_b) {
    // sRGB to XYZ
    r = sRGBtoLinear(r);
    g = sRGBtoLinear(g);
    b = sRGBtoLinear(b);
    
    float x = 0.4124564f * r + 0.3575761f * g + 0.1804375f * b;
    float y = 0.2126729f * r + 0.7151522f * g + 0.0721750f * b;
    float z = 0.0193339f * r + 0.1191920f * g + 0.9503041f * b;
    
    // XYZ to LAB
    x = labF(x / XYZ_WHITE_X);
    y = labF(y / XYZ_WHITE_Y);
    z = labF(z / XYZ_WHITE_Z);
    
    L = 116.0f * y - 16.0f;
    a = 500.0f * (x - y);
    lab_b = 200.0f * (y - z);
}

void ColorSchemes::labToRgb(float L, float a, float lab_b, float& r, float& g, float& b) {
    // LAB to XYZ
    float y = (L + 16.0f) / 116.0f;
    float x = a / 500.0f + y;
    float z = y - lab_b / 200.0f;
    
    x = XYZ_WHITE_X * labFInv(x);
    y = XYZ_WHITE_Y * labFInv(y);
    z = XYZ_WHITE_Z * labFInv(z);
    
    // XYZ to sRGB
    r =  3.2404542f * x - 1.5371385f * y - 0.4985314f * z;
    g = -0.9692660f * x + 1.8760108f * y + 0.0415560f * z;
    b =  0.0556434f * x - 0.2040259f * y + 1.0572252f * z;
    
    r = linearTosRGB(r);
    g = linearTosRGB(g);
    b = linearTosRGB(b);
    
    r = std::max(0.0f, std::min(1.0f, r));
    g = std::max(0.0f, std::min(1.0f, g));
    b = std::max(0.0f, std::min(1.0f, b));
}

QColor ColorSchemes::interpolateColorLAB(const QColor& c1, const QColor& c2, float t) {
    float r1 = c1.redF(), g1 = c1.greenF(), b1 = c1.blueF();
    float r2 = c2.redF(), g2 = c2.greenF(), b2 = c2.blueF();
    
    float L1, a1, b_1, L2, a2, b_2;
    rgbToLab(r1, g1, b1, L1, a1, b_1);
    rgbToLab(r2, g2, b2, L2, a2, b_2);
    
    float L = L1 + t * (L2 - L1);
    float a = a1 + t * (a2 - a1);
    float lab_b = b_1 + t * (b_2 - b_1);
    
    float r, g, b;
    labToRgb(L, a, lab_b, r, g, b);
    
    return QColor::fromRgbF(r, g, b);
}

QColor ColorSchemes::interpolateColor(const QColor& c1, const QColor& c2, float t, float gamma) {
    t = normalizeValue(t);
    
    if (s_perceptualCorrection) {
        return interpolateColorLAB(c1, c2, t);
    }
    
    float gammaT = gammaCorrect(t, gamma);
    
    int r = static_cast<int>(c1.red()   * (1.0f - gammaT) + c2.red()   * gammaT + 0.5f);
    int g = static_cast<int>(c1.green() * (1.0f - gammaT) + c2.green() * gammaT + 0.5f);
    int b = static_cast<int>(c1.blue()  * (1.0f - gammaT) + c2.blue()  * gammaT + 0.5f);
    
    return QColor(clampInt(r, 0, 255), clampInt(g, 0, 255), clampInt(b, 0, 255));
}

QColor ColorSchemes::interpolateFromPalette(const std::vector<ColorStop>& stops, float value) {
    if (stops.empty()) return QColor(0, 0, 0);
    if (stops.size() == 1) return stops[0].color;
    
    value = normalizeValue(value);
    
    // Найти соседние точки
    for (size_t i = 0; i < stops.size() - 1; ++i) {
        if (value >= stops[i].position && value <= stops[i + 1].position) {
            float range = stops[i + 1].position - stops[i].position;
            if (range < 1e-6f) return stops[i].color;
            
            float t = (value - stops[i].position) / range;
            return interpolateColor(stops[i].color, stops[i + 1].color, t, s_gamma);
        }
    }
    
    // Значение вне диапазона
    if (value < stops[0].position) return stops[0].color;
    return stops.back().color;
}

float ColorSchemes::contrastAdjust(float v, float contrast, float brightness) {
    v = normalizeValue(v);
    // Применяем контрастность относительно середины
    v = 0.5f + contrast * (v - 0.5f);
    // Применяем яркость
    v += brightness;
    return normalizeValue(v);
}

// Утилиты
float ColorSchemes::normalizeValue(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

int ColorSchemes::clampInt(int v, int lo, int hi) {
    return std::max(lo, std::min(v, hi));
}

float ColorSchemes::gammaCorrect(float v, float gamma) {
    return std::pow(v, 1.0f / gamma);
}

// Предопределенные схемы
std::vector<ColorStop> ColorSchemes::getSeismicStops() {
    return {
        {0.00f, QColor(0, 0, 0)},        // черный
        {0.05f, QColor(0, 0, 64)},       // очень темно-синий
        {0.10f, QColor(0, 0, 128)},      // темно-синий
        {0.15f, QColor(0, 0, 192)},      // синий
        {0.20f, QColor(0, 0, 255)},      // ярко-синий
        {0.25f, QColor(64, 64, 255)},    // светло-синий
        {0.30f, QColor(128, 128, 255)},  // голубой
        {0.35f, QColor(192, 192, 255)},  // светло-голубой
        {0.40f, QColor(255, 255, 255)},  // белый центр
        {0.45f, QColor(255, 255, 192)},  // светло-желтый
        {0.50f, QColor(255, 255, 128)},  // желтый
        {0.55f, QColor(255, 255, 64)},   // ярко-желтый
        {0.60f, QColor(255, 255, 0)},    // чистый желтый
        {0.65f, QColor(255, 192, 0)},    // оранжево-желтый
        {0.70f, QColor(255, 128, 0)},    // оранжевый
        {0.75f, QColor(255, 64, 0)},     // красно-оранжевый
        {0.80f, QColor(255, 0, 0)},      // красный
        {0.85f, QColor(192, 0, 0)},      // темно-красный
        {0.90f, QColor(128, 0, 0)},      // очень темно-красный
        {0.95f, QColor(64, 0, 0)},       // почти черный красный
        {1.00f, QColor(0, 0, 0)}         // черный
    };
}

std::vector<ColorStop> ColorSchemes::getBWRStops() {
    return {
        {0.00f, QColor(0, 0, 128)},      // темно-синий
        {0.05f, QColor(0, 0, 160)},      // синий
        {0.10f, QColor(0, 0, 192)},      // ярко-синий
        {0.15f, QColor(0, 0, 224)},      // светло-синий
        {0.20f, QColor(0, 0, 255)},      // чистый синий
        {0.25f, QColor(64, 64, 255)},    // голубой
        {0.30f, QColor(128, 128, 255)},  // светло-голубой
        {0.35f, QColor(192, 192, 255)},  // очень светло-голубой
        {0.40f, QColor(255, 255, 255)},  // белый центр
        {0.45f, QColor(255, 192, 192)},  // очень светло-красный
        {0.50f, QColor(255, 128, 128)},  // светло-красный
        {0.55f, QColor(255, 64, 64)},    // красный
        {0.60f, QColor(255, 0, 0)},      // чистый красный
        {0.65f, QColor(224, 0, 0)},      // темно-красный
        {0.70f, QColor(192, 0, 0)},      // очень темно-красный
        {0.75f, QColor(160, 0, 0)},      // почти черный красный
        {1.00f, QColor(128, 0, 0)}       // черный красный
    };
}

std::vector<ColorStop> ColorSchemes::getGrayStops() {
    return {
        {0.00f, QColor(0, 0, 0)},        // черный
        {1.00f, QColor(255, 255, 255)}   // белый
    };
}

std::vector<ColorStop> ColorSchemes::getViridisPlusStops() {
    return {
        {0.00f, QColor(68, 1, 84)},      // темно-фиолетовый
        {0.10f, QColor(72, 35, 116)},    // фиолетовый
        {0.20f, QColor(64, 67, 135)},    // сине-фиолетовый
        {0.30f, QColor(52, 94, 141)},    // синий
        {0.40f, QColor(41, 120, 142)},   // голубой
        {0.50f, QColor(32, 144, 140)},   // сине-зеленый
        {0.60f, QColor(34, 167, 132)},   // зеленый
        {0.70f, QColor(37, 188, 121)},   // желто-зеленый
        {0.80f, QColor(65, 204, 103)},   // ярко-зеленый
        {0.90f, QColor(119, 216, 67)},   // светло-зеленый
        {1.00f, QColor(253, 231, 37)}    // желтый
    };
}

std::vector<ColorStop> ColorSchemes::getRedBlueStops() {
    return {
        {0.00f, QColor(0, 0, 255)},      // синий
        {0.50f, QColor(255, 255, 255)},  // белый центр
        {1.00f, QColor(255, 0, 0)}       // красный
    };
}

std::vector<ColorStop> ColorSchemes::getPhaseStops() {
    return {
        {0.00f, QColor(255, 0, 0)},      // красный
        {0.17f, QColor(255, 255, 0)},    // желтый
        {0.33f, QColor(0, 255, 0)},      // зеленый
        {0.50f, QColor(0, 255, 255)},    // cyan
        {0.67f, QColor(0, 0, 255)},      // синий
        {0.83f, QColor(255, 0, 255)},    // magenta
        {1.00f, QColor(255, 0, 0)}       // красный (циклическая)
    };
}

std::vector<ColorStop> ColorSchemes::getAmplitudeStops() {
    return {
        {0.00f, QColor(0, 0, 0)},        // черный (нулевая амплитуда)
        {0.05f, QColor(40, 0, 80)},      // темно-фиолетовый
        {0.15f, QColor(80, 0, 160)},     // фиолетовый
        {0.30f, QColor(0, 80, 200)},     // синий
        {0.50f, QColor(0, 160, 160)},    // cyan
        {0.70f, QColor(160, 200, 0)},    // зеленый
        {0.85f, QColor(255, 160, 0)},    // оранжевый
        {1.00f, QColor(255, 255, 255)}   // белый (максимальная амплитуда)
    };
}

std::vector<ColorStop> ColorSchemes::getSpectrumStops() {
    return {
        {0.00f, QColor(255, 0, 0)},      // красный
        {0.17f, QColor(255, 255, 0)},    // желтый
        {0.33f, QColor(0, 255, 0)},      // зеленый
        {0.50f, QColor(0, 255, 255)},    // cyan
        {0.67f, QColor(0, 0, 255)},      // синий
        {0.83f, QColor(255, 0, 255)},    // magenta
        {1.00f, QColor(255, 0, 0)}       // красный
    };
}

// Улучшенные палитры для сейсмики
std::vector<ColorStop> ColorSchemes::getPetrelClassicStops() {
    return {
        {0.00f, QColor(0, 0, 90)},       // глубокий синий
        {0.15f, QColor(0, 60, 160)},     // синий
        {0.25f, QColor(0, 120, 220)},    // голубой
        {0.35f, QColor(80, 180, 255)},   // светло-голубой
        {0.45f, QColor(200, 230, 255)},  // очень светло-голубой
        {0.50f, QColor(255, 255, 255)},  // белый центр
        {0.55f, QColor(255, 230, 200)},  // очень светло-оранжевый
        {0.65f, QColor(255, 180, 80)},   // светло-оранжевый
        {0.75f, QColor(220, 120, 0)},    // оранжевый
        {0.85f, QColor(160, 60, 0)},     // красно-оранжевый
        {1.00f, QColor(90, 0, 0)}        // глубокий красный
    };
}

std::vector<ColorStop> ColorSchemes::getKingdomStops() {
    return {
        {0.00f, QColor(20, 20, 120)},    // темно-синий с оттенком
        {0.12f, QColor(0, 80, 180)},     // синий
        {0.25f, QColor(0, 140, 240)},    // ярко-синий
        {0.37f, QColor(100, 200, 255)},  // голубой
        {0.45f, QColor(180, 240, 255)},  // светло-голубой
        {0.50f, QColor(248, 248, 248)},  // почти белый
        {0.55f, QColor(255, 240, 180)},  // светло-желтый
        {0.63f, QColor(255, 200, 100)},  // желто-оранжевый
        {0.75f, QColor(240, 140, 0)},    // оранжевый
        {0.88f, QColor(180, 80, 0)},     // красно-оранжевый
        {1.00f, QColor(120, 20, 20)}     // темно-красный
    };
}

std::vector<ColorStop> ColorSchemes::getSeisWorksStops() {
    return {
        {0.00f, QColor(0, 0, 128)},      // темно-синий
        {0.20f, QColor(0, 0, 255)},      // синий
        {0.40f, QColor(0, 255, 255)},    // cyan
        {0.50f, QColor(255, 255, 255)},  // белый
        {0.60f, QColor(255, 255, 0)},    // желтый
        {0.80f, QColor(255, 0, 0)},      // красный
        {1.00f, QColor(128, 0, 0)}       // темно-красный
    };
}

// Основные методы
QColor ColorSchemes::getColor(float normalizedValue, const QString& schemeName) {
    normalizedValue = normalizeValue(normalizedValue);
    
    if (schemeName == "gray") {
        return interpolateFromPalette(getGrayStops(), normalizedValue);
    } else if (schemeName == "seismic") {
        return interpolateFromPalette(getSeismicStops(), normalizedValue);
    } else if (schemeName == "BWR") {
        return interpolateFromPalette(getBWRStops(), normalizedValue);
    } else if (schemeName == "viridis") {
        return interpolateFromPalette(getViridisPlusStops(), normalizedValue);
    } else if (schemeName == "red_blue") {
        return interpolateFromPalette(getRedBlueStops(), normalizedValue);
    } else if (schemeName == "phase") {
        return interpolateFromPalette(getPhaseStops(), normalizedValue);
    } else if (schemeName == "amplitude") {
        return interpolateFromPalette(getAmplitudeStops(), normalizedValue);
    } else if (schemeName == "spectrum") {
        return interpolateFromPalette(getSpectrumStops(), normalizedValue);
    } else if (schemeName == "petrel_classic") {
        return interpolateFromPalette(getPetrelClassicStops(), normalizedValue);
    } else if (schemeName == "kingdom") {
        return interpolateFromPalette(getKingdomStops(), normalizedValue);
    } else if (schemeName == "seisworks") {
        return interpolateFromPalette(getSeisWorksStops(), normalizedValue);
    } else {
        // Проверяем пользовательские схемы
        auto it = s_customSchemes.find(schemeName);
        if (it != s_customSchemes.end()) {
            return it->second->getColor(normalizedValue);
        }
        return interpolateFromPalette(getGrayStops(), normalizedValue);
    }
}

QColor ColorSchemes::getColorWithParams(float normalizedValue, const QString& schemeName, 
                                      float contrast, float brightness, float gamma) {
    normalizedValue = contrastAdjust(normalizedValue, contrast, brightness);
    
    // Временно изменяем глобальную гамму
    float oldGamma = s_gamma;
    s_gamma = gamma;
    
    QColor result = getColor(normalizedValue, schemeName);
    
    s_gamma = oldGamma;
    return result;
}

std::vector<QColor> ColorSchemes::getColorPalette(const QString& schemeName, int numColors) {
    std::vector<QColor> palette;
    palette.reserve(numColors);
    
    for (int i = 0; i < numColors; ++i) {
        float value = static_cast<float>(i) / (numColors - 1);
        palette.push_back(getColor(value, schemeName));
    }
    
    return palette;
}

QStringList ColorSchemes::getAvailableSchemes() {
    QStringList schemes = {
        "gray", "seismic", "BWR", "viridis", "red_blue", "phase", 
        "amplitude", "spectrum", "petrel_classic", "kingdom", "seisworks"
    };
    
    // Добавляем пользовательские схемы
    for (std::map<QString, std::unique_ptr<ColorScheme>>::const_iterator it = s_customSchemes.begin();
         it != s_customSchemes.end(); ++it) {
        schemes.append(it->first);
    }
    
    return schemes;
}

bool ColorSchemes::hasScheme(const QString& schemeName) {
    return getAvailableSchemes().contains(schemeName);
}

// Методы для работы с пользовательскими схемами
void ColorSchemes::addCustomScheme(const ColorScheme& scheme) {
    s_customSchemes[scheme.name] = std::unique_ptr<ColorScheme>(new ColorScheme(scheme));
}

void ColorSchemes::removeCustomScheme(const QString& name) {
    s_customSchemes.erase(name);
}

ColorScheme* ColorSchemes::getScheme(const QString& name) {
    auto it = s_customSchemes.find(name);
    if (it != s_customSchemes.end()) {
        return it->second.get();
    }
    return nullptr;
}

// Специфичные для сейсмики методы
QColor ColorSchemes::getSeismicColor(float amplitude, float rms, bool bipolar) {
    if (bipolar) {
        // Биполярные данные: положительные и отрицательные амплитуды
        float normalized = amplitude / (rms * 3.0f); // ±3 RMS как диапазон
        normalized = (normalized + 1.0f) / 2.0f;     // 0-1 диапазон
        return interpolateFromPalette(getPetrelClassicStops(), normalized);
    } else {
        // Униполярные данные: только положительные амплитуды
        float normalized = std::abs(amplitude) / (rms * 3.0f);
        return interpolateFromPalette(getAmplitudeStops(), normalized);
    }
}

std::vector<QColor> ColorSchemes::generateSeismicPalette(int numColors, float centerBias) {
    std::vector<QColor> palette;
    palette.reserve(numColors);
    
    for (int i = 0; i < numColors; ++i) {
        float value = static_cast<float>(i) / (numColors - 1);
        
        // Применяем смещение центра для лучшего восприятия
        if (centerBias != 0.5f) {
            value = std::pow(value, centerBias);
        }
        
        palette.push_back(getColor(value, "petrel_classic"));
    }
    
    return palette;
}
