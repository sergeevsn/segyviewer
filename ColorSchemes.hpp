#ifndef COLORSCHEMES_HPP
#define COLORSCHEMES_HPP

#include <QColor>
#include <QString>
#include <QStringList>
#include <vector>
#include <map>
#include <memory>

struct ColorStop {
    float position;  // 0.0 - 1.0
    QColor color;
    
    ColorStop(float pos, const QColor& c) : position(pos), color(c) {}
};

class ColorScheme {
public:
    QString name;
    std::vector<ColorStop> stops;
    bool cyclic = false;  // для циклических схем
    float contrast = 1.0f;  // множитель контрастности
    float brightness = 0.0f;  // сдвиг яркости
    
    ColorScheme(const QString& n) : name(n) {}
    void addStop(float pos, const QColor& color);
    QColor getColor(float value) const;
};

class ColorSchemes {
public:
    // Основные методы
    static QColor getColor(float normalizedValue, const QString& schemeName);
    static QColor getColorWithParams(float normalizedValue, const QString& schemeName, 
                                   float contrast = 1.0f, float brightness = 0.0f, float gamma = 1.0f);
    static std::vector<QColor> getColorPalette(const QString& schemeName, int numColors);
    static QStringList getAvailableSchemes();
    static bool hasScheme(const QString& schemeName);
    
    // Расширенные возможности
    static void setCustomGamma(float gamma) { s_gamma = gamma; }
    static float getCustomGamma() { return s_gamma; }
    static void enablePerceptualCorrection(bool enable) { s_perceptualCorrection = enable; }
    
    // Методы для работы с пользовательскими схемами
    static void addCustomScheme(const ColorScheme& scheme);
    static void removeCustomScheme(const QString& name);
    static ColorScheme* getScheme(const QString& name);
    
    // Специфичные для сейсмики методы
    static QColor getSeismicColor(float amplitude, float rms = 1.0f, bool bipolar = true);
    static std::vector<QColor> generateSeismicPalette(int numColors, float centerBias = 0.5f);
    
    // Утилиты (публичные для доступа из ColorScheme)
    static float normalizeValue(float v);
    static float contrastAdjust(float v, float contrast, float brightness);
    static QColor interpolateFromPalette(const std::vector<ColorStop>& stops, float value);
    
private:
    // Улучшенные базовые методы
    static QColor interpolateColor(const QColor& c1, const QColor& c2, float t, float gamma = 1.0f);
    static QColor interpolateColorLAB(const QColor& c1, const QColor& c2, float t);
    
    // Перцептивная коррекция
    static QColor perceptualCorrection(const QColor& color);
    static void rgbToLab(float r, float g, float b, float& L, float& a, float& lab_b);
    static void labToRgb(float L, float a, float lab_b, float& r, float& g, float& b);
    
    // Утилиты
    static int clampInt(int v, int lo, int hi);
    static float gammaCorrect(float v, float gamma);
    
    // Предопределенные схемы (улучшенные)
    static std::vector<ColorStop> getSeismicStops();
    static std::vector<ColorStop> getBWRStops();
    static std::vector<ColorStop> getGrayStops();
    static std::vector<ColorStop> getViridisPlusStops();  // улучшенная версия
    static std::vector<ColorStop> getRedBlueStops();     // классическая сейсмическая
    static std::vector<ColorStop> getPhaseStops();       // для фазовых данных
    static std::vector<ColorStop> getAmplitudeStops();   // для амплитудных данных
    static std::vector<ColorStop> getSpectrumStops();    // радужная схема
    
    // Современные схемы для геофизики
    static std::vector<ColorStop> getPetrelClassicStops();
    static std::vector<ColorStop> getKingdomStops();
    static std::vector<ColorStop> getSeisWorksStops();
    
    // Статические переменные
    static float s_gamma;
    static bool s_perceptualCorrection;
    static std::map<QString, std::unique_ptr<ColorScheme>> s_customSchemes;
};

#endif // COLORSCHEMES_HPP
