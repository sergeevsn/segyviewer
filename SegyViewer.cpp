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
      globalStatsComputed(false),
      gridEnabled(true)  // По умолчанию сетка включена
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
    p.fillRect(rect(), Qt::white); // Белый фон вместо черного

    if (!dataManager) {
        p.setPen(Qt::black); // Черный текст на белом фоне
        p.drawText(rect(), Qt::AlignCenter, "No data loaded");
        return;
    }

    auto traces = dataManager->getTracesRange(startTraceIndex, tracesPerPage);
    if (traces.empty()) {
        p.setPen(Qt::black); // Черный текст на белом фоне
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

    // Определяем размеры для осей - асимметричные отступы
    const int leftMargin = 80;   // Отступ слева для подписей времени
    const int bottomMargin = 80; // Отступ снизу для подписей трасс
    const int rightMargin = 20;  // Минимальный отступ справа
    const int topMargin = 20;    // Минимальный отступ сверху
    const int tickLength = 5;    // Длина делений
    const int labelSpacing = 100; // Интервал между подписями
    
    QRect imageRect(leftMargin, topMargin, 
                   width() - leftMargin - rightMargin, 
                   height() - topMargin - bottomMargin);
    
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
    p.drawImage(imageRect, img);
    
    // Вычисляем шаги для подписей
    int traceStep = std::max(1, traceCount / (imageRect.width() / labelSpacing));
    
    // Вычисляем оптимальный шаг времени кратный 250мс
    float dt = dataManager->getSampleInterval(); // в микросекундах
    float totalTimeMs = samplesToShow * dt; // общее время в миллисекундах
    
    // Вычисляем оптимальный шаг времени в миллисекундах
    int timeStepMs = calculateOptimalTimeStep(totalTimeMs, imageRect.height(), labelSpacing);
    
    // Рисуем оси
    p.setPen(Qt::black); // Черные оси на белом фоне
    p.setFont(QFont("Arial", 8));
    
    // Рисуем сетку только если она включена
    if (gridEnabled) {
        p.setPen(QPen(Qt::lightGray, 1, Qt::DotLine)); // Светло-серая сетка
        
        // Вертикальные линии сетки (трассы)
        for (int i = 0; i < traceCount; i += traceStep) {
            int x = leftMargin + (i * imageRect.width()) / traceCount;
            p.drawLine(x, topMargin, x, height() - bottomMargin);
        }
        
        // Горизонтальные линии сетки (время) - используем оптимальный шаг
        for (int timeMs = 0; timeMs <= totalTimeMs; timeMs += timeStepMs) {
            float timeRatio = timeMs / totalTimeMs;
            int y = topMargin + static_cast<int>(timeRatio * imageRect.height());
            p.drawLine(leftMargin, y, width() - rightMargin, y);
        }
    }
    
    // Рисуем основные оси
    p.setPen(QPen(Qt::black, 2, Qt::SolidLine)); // Черные оси
    
    // Ось трасс (X) - по нижнему краю картинки
    p.drawLine(leftMargin, height() - bottomMargin, width() - rightMargin, height() - bottomMargin);
    
    // Ось времени (Y) - по левому краю картинки
    p.drawLine(leftMargin, topMargin, leftMargin, height() - bottomMargin);
    
    // Подписи оси трасс - под картинкой
    for (int i = 0; i < traceCount; i += traceStep) {
        int x = leftMargin + (i * imageRect.width()) / traceCount;
        int traceIndex = startTraceIndex + i;
        
        // Деление - от нижнего края картинки вниз
        p.drawLine(x, height() - bottomMargin, x, height() - bottomMargin + tickLength);
        
        // Подпись - под делением
        QString label = QString::number(traceIndex);
        QRect textRect(x - 20, height() - bottomMargin + tickLength + 5, 40, 20);
        p.drawText(textRect, Qt::AlignCenter, label);
    }
    
    // Подписи оси времени - используем временные интервалы кратные 250мс
    for (int timeMs = 0; timeMs <= totalTimeMs; timeMs += timeStepMs) {
        // Вычисляем позицию Y для данного времени
        float timeRatio = timeMs / totalTimeMs;
        int y = topMargin + static_cast<int>(timeRatio * imageRect.height());
        
        // Деление - от левого края картинки влево
        p.drawLine(leftMargin - tickLength, y, leftMargin, y);
        
        // Подпись времени - слева от деления, всегда в миллисекундах
        QString timeLabel = QString::number(timeMs) + " ms";
        
        QRect textRect(0, y - 10, leftMargin - tickLength - 5, 20);
        p.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, timeLabel);
    }
    
    // Подписи осей
    p.setFont(QFont("Arial", 10, QFont::Bold));
    
    // Подпись оси трасс - под картинкой, ниже оси
    QRect xAxisLabelRect(leftMargin, height() - bottomMargin + 40, imageRect.width(), 20);
    p.drawText(xAxisLabelRect, Qt::AlignCenter, "Trace Number");
    
    // Подпись оси времени - слева от картинки, с большим отступом
    QRect yAxisLabelRect(0, topMargin, 20, imageRect.height());
    p.save();
    p.translate(15, topMargin + imageRect.height() / 2); // Увеличиваем отступ с 5 до 15
    p.rotate(-90);
    p.drawText(QRect(-50, -10, 100, 20), Qt::AlignCenter, "Time (ms)"); // Добавляем единицы измерения
    p.restore();
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

    // Учитываем отступы для осей
    const int leftMargin = 80;   // Отступ слева для подписей времени
    const int bottomMargin = 80; // Отступ снизу для подписей трасс
    const int rightMargin = 20;  // Минимальный отступ справа
    const int topMargin = 20;    // Минимальный отступ сверху
    int width = this->width() - leftMargin - rightMargin;
    int height = this->height() - topMargin - bottomMargin;
    
    if (width <= 0 || height <= 0) return;
    
    int traceCount = traces.size();
    int maxSamples = 0;
    
    for (const auto& trace : traces) {
        maxSamples = std::max(maxSamples, static_cast<int>(trace.size()));
    }
    
    if (maxSamples == 0) return;

    int samplesToShow = (samplesPerPage > 0) ? std::min(samplesPerPage, maxSamples) : maxSamples;

    double pixelWidth = static_cast<double>(width) / traceCount;
    double pixelHeight = static_cast<double>(height) / samplesToShow;

    // Корректируем координаты мыши с учетом отступов
    int mouseX = event->x() - leftMargin;
    int mouseY = event->y() - topMargin;
    
    if (mouseX < 0 || mouseX >= width || mouseY < 0 || mouseY >= height) return;

    int traceIndex = static_cast<int>(mouseX / pixelWidth);
    int sampleIndex = static_cast<int>(mouseY / pixelHeight) + startSampleIndex;
    
    if (traceIndex < 0 || traceIndex >= traceCount) return;
    if (sampleIndex < 0 || sampleIndex >= static_cast<int>(traces[traceIndex].size())) return;

    float amp = traces[traceIndex][sampleIndex];
    emit traceInfoUnderCursor(startTraceIndex + traceIndex, sampleIndex, amp);
}

int SegyViewer::calculateOptimalTimeStep(float totalTimeMs, int height, int labelSpacing) const {
    // Желаемый интервал между подписями в пикселях
    int desiredPixelSpacing = labelSpacing;
    
    // Вычисляем, сколько подписей поместится на оси
    int maxLabels = height / desiredPixelSpacing;
    if (maxLabels < 2) maxLabels = 2; // Минимум 2 подписи
    
    // Вычисляем базовый шаг времени
    float baseTimeStep = totalTimeMs / maxLabels;
    
    // Округляем до ближайшего кратного 250мс
    int roundedStepMs = static_cast<int>(std::round(baseTimeStep / 250.0f)) * 250;
    
    // Убеждаемся, что шаг не слишком маленький
    if (roundedStepMs < 250) roundedStepMs = 250;
    
    // Убеждаемся, что шаг не слишком большой (не более 1/4 от общего времени)
    if (roundedStepMs > totalTimeMs / 4) {
        roundedStepMs = static_cast<int>(totalTimeMs / 4);
    }
    
    // Возвращаем шаг в миллисекундах, а не в сэмплах
    return roundedStepMs;
}
