#include "SegyViewer.hpp"
#include "SegyDataManager.hpp"
#include "ColorSchemes.hpp"
#include <QPainter>
#include <QMouseEvent>
#include <algorithm>
#include <limits>
#include <cmath>

SegyViewer::SegyViewer(QWidget* parent)
    : QWidget(parent),
      dataManager(nullptr),
      pageIndex(0),
      startTraceIndex(0),
      tracesPerPage(1000),
      samplesPerPage(0),
      startSampleIndex(0),
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
    colorMapValid = false; // нужно пересчитать цветовую карту
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

    if (!colorMapValid) {
        updateColorMap();
    }

    int traceCount = traces.size();
    int maxSamples = 0;
    for (const auto& tr : traces)
        maxSamples = std::max(maxSamples, static_cast<int>(tr.size()));
    if (maxSamples == 0) return;

    int samplesToShow = (samplesPerPage > 0) ? std::min(samplesPerPage, maxSamples) : maxSamples;

    // Создаём QImage и сразу заполняем его данными
    QImage img(traceCount, samplesToShow, QImage::Format_ARGB32);
    for (int y = 0; y < samplesToShow; ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < traceCount; ++x) {
            float amp = traces[x][y + startSampleIndex];
            line[x] = amplitudeToRgb(amp);
        }
    }

    // Включаем сглаживание при масштабировании (как в matplotlib imshow)
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.drawImage(rect(), img);
}

void SegyViewer::updateColorMap() {
    if (!dataManager) return;

    if (!globalStatsComputed) {
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

            if (std::abs(maxAmplitude - minAmplitude) < 1e-6) {
                maxAmplitude = minAmplitude + 1.0f;
            }
            
            globalStatsComputed = true;
        }
    }

    // Заполняем LUT (256 цветов)
    lut.resize(256);
    for (int i = 0; i < 256; ++i) {
        float norm = i / 255.0f;
        QColor c = ColorSchemes::getColor(norm, colorScheme);
        lut[i] = c.rgba();
    }

    colorMapValid = true;
}

uint32_t SegyViewer::amplitudeToRgb(float amplitude) const {
    if (!std::isfinite(amplitude)) {
        return qRgba(128, 128, 128, 255); // серый для NaN
    }

    float range = maxAmplitude - minAmplitude;
    float center = 0.5f * (maxAmplitude + minAmplitude);
    float effectiveRange = range / gain;
    float minEff = center - effectiveRange * 0.5f;
    float maxEff = center + effectiveRange * 0.5f;

    float norm = (amplitude - minEff) / effectiveRange;
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;

    int idx = static_cast<int>(norm * 255.0f);
    return lut[idx];
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

    int samplesToShow = (samplesPerPage > 0) ? std::min(samplesPerPage, maxSamples) : maxSamples;

    double pixelWidth = static_cast<double>(width) / traceCount;
    double pixelHeight = static_cast<double>(height) / samplesToShow;

    int traceIndex = static_cast<int>(event->x() / pixelWidth);
    int sampleIndex = static_cast<int>(event->y() / pixelHeight) + startSampleIndex;
    
    if (traceIndex < 0 || traceIndex >= traceCount) return;
    if (sampleIndex < 0 || sampleIndex >= static_cast<int>(traces[traceIndex].size())) return;

    float amp = traces[traceIndex][sampleIndex];
    emit traceInfoUnderCursor(startTraceIndex + traceIndex, sampleIndex, amp);
}
