#pragma once
#include <QWidget>
#include <QString>
#include <vector>
#include <limits>
#include <cstdint>

class SegyDataManager;

class SegyViewer : public QWidget {
    Q_OBJECT
public:
    explicit SegyViewer(QWidget* parent = nullptr);
    void setDataManager(SegyDataManager* manager);
    void setCurrentPage(int page);
    void setStartTrace(int traceIndex);
    int currentPage() const { return pageIndex; }
    int startTrace() const { return startTraceIndex; }

    void setTracesPerPage(int tpp) { tracesPerPage = tpp; }
    int getTracesPerPage() const { return tracesPerPage; }
    void setSamplesPerPage(int spp) { samplesPerPage = spp; colorMapValid = false; }
    int getSamplesPerPage() const { return samplesPerPage; }
    void setStartSample(int sampleIndex) { startSampleIndex = sampleIndex; colorMapValid = false; }
    int getStartSample() const { return startSampleIndex; }
    void setColorScheme(const QString& scheme);
    void setGain(float g) { gain = g; colorMapValid = false; }
    void setGridEnabled(bool enabled) { gridEnabled = enabled; update(); }
    
    // Новые методы для цветовых схем
    void setGamma(float g);
    void setContrast(float c);
    void setBrightness(float b);
    void setPerceptualCorrection(bool enabled);

signals:
    void traceInfoUnderCursor(int traceIndex, int sampleIndex, float amplitude);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void updateColorMap();
    uint32_t amplitudeToRgb(float amplitude) const; // быстрый доступ через LUT
    int calculateOptimalTimeStep(float totalTimeMs, int height, int labelSpacing) const; // расчет оптимального шага времени

    SegyDataManager* dataManager;
    int pageIndex;
    int startTraceIndex;   // Начальный индекс трассы для отображения
    int tracesPerPage;
    int samplesPerPage;    // Количество сэмплов на страницу (0 = все сэмплы)
    int startSampleIndex;  // Начальный индекс сэмпла для отображения
    QString colorScheme;
    
    // Новые параметры для цветовых схем
    float gamma;
    float contrast;
    float brightness;
    bool perceptualCorrection;
    
    // Для цветовой карты
    float minAmplitude;
    float maxAmplitude;
    bool colorMapValid;
    float gain;
    bool globalStatsComputed;
    bool gridEnabled;      // Включена ли сетка

    std::vector<uint32_t> lut; // таблица цветов (256 уровней)
};
