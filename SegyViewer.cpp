#include "SegyViewer.hpp"
#include "SegyDataManager.hpp"
#include <QPainter>
#include <QMouseEvent>
#include <algorithm>
#include <limits>
#include <iostream>
#include <cmath>

SegyViewer::SegyViewer(QWidget* parent)
    : QWidget(parent),
      dataManager(nullptr),
      pageIndex(0),
      startTraceIndex(0),
      tracesPerPage(200),
      colorScheme("Grayscale"),
      minAmplitude(0.0f),
      maxAmplitude(1.0f),
      colorMapValid(false),
      gain(5.0f),

      globalStatsComputed(false)
{
    setMouseTracking(true);
}

void SegyViewer::setDataManager(SegyDataManager* manager) {
    dataManager = manager;
}

void SegyViewer::setColorScheme(const QString& scheme) {
    colorScheme = scheme;
    colorMapValid = false; // Нужно пересчитать цветовую карту
    update();
}

void SegyViewer::setCurrentPage(int page) {
    if (dataManager) {
        int maxPage = (dataManager->traceCount() - 1) / tracesPerPage;
        if (page < 0) page = 0;
        if (page > maxPage) page = maxPage;
        pageIndex = page;
        startTraceIndex = page * tracesPerPage;
        update();
    }
}

void SegyViewer::setStartTrace(int traceIndex) {
    if (dataManager) {
        int maxTrace = dataManager->traceCount() - tracesPerPage;
        if (traceIndex < 0) traceIndex = 0;
        if (traceIndex > maxTrace) traceIndex = maxTrace;
        startTraceIndex = traceIndex;
        pageIndex = traceIndex / tracesPerPage;
        
        // Инвалидируем цветовую карту при изменении трасс
        colorMapValid = false;
        
        update();
    }
}

void SegyViewer::paintEvent(QPaintEvent* /*event*/) {
    QPainter p(this);
    p.fillRect(rect(), Qt::black);

    if (!dataManager) {
        p.setPen(Qt::white);
        p.drawText(rect(), Qt::AlignCenter, "No data loaded");
        return;
    }

    auto traces = dataManager->getTracesRange(startTraceIndex, tracesPerPage);
    if (traces.empty()) {
        p.setPen(Qt::white);
        p.drawText(rect(), Qt::AlignCenter, "No traces to display");
        return;
    }

    // Обновляем цветовую карту если нужно
    if (!colorMapValid) {
        updateColorMap();
    }

    int width = this->width();
    int height = this->height();
    int traceCount = traces.size();
    int maxSamples = 0;
    
    // Находим максимальное количество сэмплов
    for (const auto& trace : traces) {
        maxSamples = std::max(maxSamples, static_cast<int>(trace.size()));
    }
    
    if (maxSamples == 0) return;

    // Вычисляем размеры пикселей
    double pixelWidth = static_cast<double>(width) / traceCount;
    double pixelHeight = static_cast<double>(height) / maxSamples;

    // Рисуем растровое изображение с интерполяцией на разрешение экрана
    
    // Интерполируем данные на разрешение экрана
    for (int screenX = 0; screenX < width; ++screenX) {
        // Находим соответствующие трассы для этого пикселя экрана
        double tracePos = (screenX * traceCount) / static_cast<double>(width);
        int traceIdx1 = static_cast<int>(tracePos);
        int traceIdx2 = std::min(traceIdx1 + 1, traceCount - 1);
        double weight = tracePos - traceIdx1;
        
        for (int screenY = 0; screenY < height; ++screenY) {
            // Находим соответствующие сэмплы для этого пикселя экрана
            double samplePos = (screenY * maxSamples) / static_cast<double>(height);
            int sampleIdx1 = static_cast<int>(samplePos);
            int sampleIdx2 = std::min(sampleIdx1 + 1, maxSamples - 1);
            double sampleWeight = samplePos - sampleIdx1;
            
            // Билинейная интерполяция
            float amplitude = 0.0f;
            int validSamples = 0;
            
            // Интерполируем между трассами
            for (int t = traceIdx1; t <= traceIdx2; ++t) {
                if (t >= 0 && t < traceCount) {
                    const auto& trace = traces[t];
                    double traceWeight = (t == traceIdx1) ? (1.0 - weight) : weight;
                    
                    // Интерполируем между сэмплами
                    for (int s = sampleIdx1; s <= sampleIdx2; ++s) {
                        if (s >= 0 && s < static_cast<int>(trace.size())) {
                            double sampleWeight2 = (s == sampleIdx1) ? (1.0 - sampleWeight) : sampleWeight;
                            amplitude += trace[s] * traceWeight * sampleWeight2;
                            validSamples++;
                        }
                    }
                }
            }
            
            if (validSamples > 0) {
                amplitude /= validSamples;
                QColor color = amplitudeToColor(amplitude);
                p.setPen(color);
                p.drawPoint(screenX, screenY);
            }
        }
    }
}

void SegyViewer::updateColorMap() {
    if (!dataManager) return;

    if (!globalStatsComputed) {
        // Вычисляем глобальные статистики один раз на основе первых 1000 трасс
        auto traces = dataManager->getTracesRange(0, 1000);
        if (!traces.empty()) {
            minAmplitude = std::numeric_limits<float>::max();
            maxAmplitude = std::numeric_limits<float>::lowest();

            for (const auto& trace : traces) {
                for (float amplitude : trace) {
                    if (std::isfinite(amplitude)) {
                        minAmplitude = std::min(minAmplitude, amplitude);
                        maxAmplitude = std::max(maxAmplitude, amplitude);
                    }
                }
            }

            // Если все значения одинаковые, устанавливаем небольшой диапазон
            if (std::abs(maxAmplitude - minAmplitude) < 1e-6) {
                maxAmplitude = minAmplitude + 1.0f;
            }
            
            globalStatsComputed = true;
            std::cout << "Global normalization computed: min=" << minAmplitude 
                      << ", max=" << maxAmplitude << std::endl;
        }
    }

    colorMapValid = true;
}

QColor SegyViewer::amplitudeToColor(float amplitude) const {
    if (!colorMapValid) return Qt::black;
    
    // Проверка на некорректные данные
    if (!std::isfinite(amplitude)) {
        return Qt::gray;
    }
    
    // Вычисляем эффективные границы с учётом gain
    // gain = 1.0 - используем полный диапазон [minAmplitude, maxAmplitude]
    // gain > 1.0 - сужаем диапазон, отсекая крайние значения
    float range = maxAmplitude - minAmplitude;
    float centerAmplitude = (maxAmplitude + minAmplitude) * 0.5f;
    
    // Чем больше gain, тем меньше эффективный диапазон
    float effectiveRange = range / gain;
    float effectiveMin = centerAmplitude - effectiveRange * 0.5f;
    float effectiveMax = centerAmplitude + effectiveRange * 0.5f;
    
    // Нормализуем амплитуду относительно эффективного диапазона
    float normalized = 0.5f; // по умолчанию средний серый
    if (effectiveRange > 0.0f) {
        normalized = (amplitude - effectiveMin) / effectiveRange;
    }
    
    // Ограничиваем значение в диапазоне [0, 1]
    normalized = std::max(0.0f, std::min(1.0f, normalized));
    
    // Преобразуем в цвет
    int intensity = static_cast<int>(normalized * 255.0f + 0.5f);
    intensity = std::max(0, std::min(255, intensity));
    
    if (colorScheme == "Grayscale") {
        return QColor(intensity, intensity, intensity);
    } else if (colorScheme == "Red") {
        return QColor(intensity, 0, 0);
    } else if (colorScheme == "White") {
        return QColor(intensity, intensity, intensity);
    } else {
        // По умолчанию grayscale
        return QColor(intensity, intensity, intensity);
    }
}

void SegyViewer::mouseMoveEvent(QMouseEvent* event) {
    if (!dataManager) return;

    auto traces = dataManager->getTracesRange(startTraceIndex, tracesPerPage);
    if (traces.empty()) return;

    int width = this->width();
    int height = this->height();
    int traceCount = traces.size();
    int maxSamples = 0;
    
    for (const auto& trace : traces) {
        maxSamples = std::max(maxSamples, static_cast<int>(trace.size()));
    }
    
    if (maxSamples == 0) return;

    double pixelWidth = static_cast<double>(width) / traceCount;
    double pixelHeight = static_cast<double>(height) / maxSamples;

    int traceIndex = static_cast<int>(event->x() / pixelWidth);
    int sampleIndex = static_cast<int>(event->y() / pixelHeight);
    
    if (traceIndex < 0 || traceIndex >= traceCount) return;
    if (sampleIndex < 0 || sampleIndex >= static_cast<int>(traces[traceIndex].size())) return;

    float amp = traces[traceIndex][sampleIndex];
    emit traceInfoUnderCursor(startTraceIndex + traceIndex, sampleIndex, amp);
}

