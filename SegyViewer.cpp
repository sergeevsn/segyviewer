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
      gain(1.0f),
      globalStatsComputed(false),
      gridEnabled(false),  // По умолчанию сетка отключена
      gamma(1.0f),         // По умолчанию стандартная гамма
      contrast(1.0f),      // По умолчанию без изменения контрастности
      brightness(0.0f),    // По умолчанию без изменения яркости
      perceptualCorrection(false),  // По умолчанию перцептивная коррекция отключена
      isZooming(false),
      hasZoomSelection(false),
      originalStartTrace(0),
      originalStartSample(0),
      originalTracesPerPage(1000),
      originalSamplesPerPage(0),
      isZoomed(false),
      zoomUpdateTimer(new QTimer(this)),
      percentilesComputed(false),
      effectiveMinAmplitude(0.0f),
      effectiveMaxAmplitude(1.0f)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent, false); // Отключаем оптимизацию перерисовки
    
    // Настраиваем таймер для обновления зума
    zoomUpdateTimer->setInterval(16); // ~60 FPS
    connect(zoomUpdateTimer, &QTimer::timeout, this, [this]() {
        if (isZooming) {
            update();
        }
    });
}

void SegyViewer::setDataManager(SegyDataManager* manager) {
    dataManager = manager;
}

void SegyViewer::setColorScheme(const QString& scheme) {
    colorScheme = scheme;
    colorMapValid = false; // нужно пересчитать цветовую карту
    update();
}

void SegyViewer::setGamma(float g) {
    gamma = g;
    colorMapValid = false;
    update();
}

void SegyViewer::setContrast(float c) {
    contrast = c;
    colorMapValid = false;
    update();
}

void SegyViewer::setBrightness(float b) {
    brightness = b;
    colorMapValid = false;
    update();
}

void SegyViewer::setPerceptualCorrection(bool enabled) {
    perceptualCorrection = enabled;
    colorMapValid = false;
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

    // samplesPerPage теперь интерпретируется как время в миллисекундах
    int samplesToShow;
    if (samplesPerPage > 0) {
        // Конвертируем время в количество сэмплов
        float dt = dataManager->getSampleInterval(); // в микросекундах
        int timeInSamples = static_cast<int>(samplesPerPage / dt);
        samplesToShow = std::min(timeInSamples, maxSamples);
    } else {
        samplesToShow = maxSamples; // 0 означает "все время"
    }
    
    // Предотвращаем зависание при очень маленьких значениях - минимум 100 сэмплов
    if (samplesToShow > 0 && samplesToShow < 100) {
        samplesToShow = std::min(100, maxSamples);
    }

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
    
    // Ограничиваем размер изображения для производительности
    const int maxImageHeight = 2000; // Максимальная высота изображения
    int actualSamplesToRender = std::min(samplesToShow, maxImageHeight);
    
    // Создаём QImage в нужном размере без суперсэмплинга
    QImage img(traceCount, actualSamplesToRender, QImage::Format_ARGB32);
    
    // Вычисляем шаг для пропуска сэмплов, если нужно
    double sampleStep = 1.0;
    if (samplesToShow > maxImageHeight) {
        sampleStep = static_cast<double>(samplesToShow) / maxImageHeight;
    }
    
    for (int y = 0; y < actualSamplesToRender; ++y) {
        // Заполняем строку с суперсэмплингом
        for (int x = 0; x < traceCount; ++x) {
            // Вычисляем индекс сэмпла с учетом шага
            int sampleIndex = startSampleIndex + static_cast<int>(y * sampleStep);
            if (sampleIndex >= static_cast<int>(traces[x].size())) {
                sampleIndex = traces[x].size() - 1;
            }
            float amp = traces[x][sampleIndex];
            QRgb color = amplitudeToRgb(amp);
            
            // Заполняем пиксель напрямую без суперсэмплинга
            img.setPixel(x, y, color);
        }
    }

    // Рисуем изображение напрямую без суперсэмплинга
    p.drawImage(imageRect, img, QRect(0, 0, img.width(), img.height()));
    
    // Вычисляем шаги для подписей
    int traceStep = std::max(1, traceCount / (imageRect.width() / labelSpacing));
    
    // Вычисляем оптимальный шаг времени кратный 250мс
    float dt = dataManager->getSampleInterval(); // в микросекундах
    float totalTimeMs = (samplesToShow - 1) * dt; // общее время в миллисекундах (сэмплы от 0 до samplesToShow-1)
    
    // Вычисляем оптимальный шаг времени в миллисекундах
    int timeStepMs = calculateOptimalTimeStep(totalTimeMs, imageRect.height(), labelSpacing);
    
    // Рисуем оси
    p.setPen(Qt::black); // Черные оси на белом фоне
    p.setFont(QFont("Arial", 8));
    
    // Рисуем сетку только если она включена
    if (gridEnabled) {
        p.setPen(QPen(Qt::black, 1, Qt::DotLine)); // Тонкая черная сетка
        
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
    
    // Рисуем прямоугольник выделения для зума
    drawSelectionRect(p);
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
    
    // Вычисляем перцентили и эффективные границы
    computePercentiles();
    updateEffectiveAmplitudeRange();

    // Заполняем LUT (1024 цвета для лучшего качества)
    const int colorMapSize = 1024;
    lut.resize(colorMapSize);
    
    // Настраиваем глобальные параметры ColorSchemes
    ColorSchemes::setCustomGamma(gamma);
    ColorSchemes::enablePerceptualCorrection(perceptualCorrection);
    
    for (int i = 0; i < colorMapSize; ++i) {
        float norm = i / static_cast<float>(colorMapSize - 1);
        QColor c = ColorSchemes::getColorWithParams(norm, colorScheme, contrast, brightness, gamma);
        lut[i] = c.rgba();
    }

    colorMapValid = true;
}

void SegyViewer::computePercentiles() {
    if (!dataManager || percentilesComputed) return;
    
    // Получаем все амплитуды для вычисления перцентилей
    auto traces = dataManager->getTracesRange(0, 1000); // Используем первые 1000 трасс для статистики
    if (traces.empty()) return;
    
    std::vector<float> allAmplitudes;
    allAmplitudes.reserve(1000 * 1000); // Предварительно выделяем память
    
    for (const auto& trace : traces) {
        for (float amplitude : trace) {
            if (std::isfinite(amplitude)) {
                allAmplitudes.push_back(amplitude);
            }
        }
    }
    
    if (allAmplitudes.empty()) return;
    
    // Сортируем амплитуды для вычисления перцентилей
    std::sort(allAmplitudes.begin(), allAmplitudes.end());
    
    // Вычисляем перцентили от 0 до 100 с шагом 0.1
    amplitudePercentiles.resize(1001); // 0.0, 0.1, 0.2, ..., 100.0
    for (int i = 0; i <= 1000; ++i) {
        float percentile = i / 10.0f; // 0.0, 0.1, 0.2, ..., 100.0
        int index = static_cast<int>((percentile / 100.0f) * (allAmplitudes.size() - 1));
        if (index >= allAmplitudes.size()) index = allAmplitudes.size() - 1;
        amplitudePercentiles[i] = allAmplitudes[index];
    }
    
    percentilesComputed = true;
}

void SegyViewer::updateEffectiveAmplitudeRange() {
    if (!percentilesComputed) {
        computePercentiles();
    }
    
    if (!percentilesComputed) return;
    
    // Применяем формулу: gain = [gain - 1.0, 101 - gain]
    float lowerPercentile = std::max(0.0f, gain - 1.0f);
    float upperPercentile = std::min(100.0f, 101.0f - gain);
    
    // Преобразуем проценты в индексы (0.1% = 1, 1.0% = 10, 99.0% = 990)
    int lowerIndex = static_cast<int>(lowerPercentile * 10.0f);
    int upperIndex = static_cast<int>(upperPercentile * 10.0f);
    
    // Ограничиваем индексы
    lowerIndex = std::max(0, std::min(lowerIndex, 1000));
    upperIndex = std::max(lowerIndex, std::min(upperIndex, 1000));
    
    // Получаем эффективные границы амплитуд
    effectiveMinAmplitude = amplitudePercentiles[lowerIndex];
    effectiveMaxAmplitude = amplitudePercentiles[upperIndex];
    
    // Защита от одинаковых значений
    if (std::abs(effectiveMaxAmplitude - effectiveMinAmplitude) < 1e-6) {
        effectiveMaxAmplitude = effectiveMinAmplitude + 1.0f;
    }
}

uint32_t SegyViewer::amplitudeToRgb(float amplitude) const {
    if (!std::isfinite(amplitude)) {
        return qRgba(128, 128, 128, 255); // серый для NaN
    }

    // Используем эффективные границы, основанные на перцентилях
    float range = effectiveMaxAmplitude - effectiveMinAmplitude;
    if (range < 1e-6) range = 1.0f; // Защита от деления на ноль
    
    float norm = (amplitude - effectiveMinAmplitude) / range;
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;

    int idx = static_cast<int>(norm * (lut.size() - 1));
    if (idx >= lut.size()) idx = lut.size() - 1;
    return lut[idx];
}

void SegyViewer::mouseMoveEvent(QMouseEvent* event) {
    if (!dataManager) return;

    // Если происходит выделение области для зума, обновляем конечную точку
    if (isZooming) {
        zoomEnd = event->pos();
        update();
        repaint(); // Принудительная перерисовка
        return;
    }

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

    // samplesPerPage теперь интерпретируется как время в миллисекундах
    int samplesToShow;
    if (samplesPerPage > 0) {
        // Конвертируем время в количество сэмплов
        float dt = dataManager->getSampleInterval(); // в микросекундах
        int timeInSamples = static_cast<int>(samplesPerPage / dt);
        samplesToShow = std::min(timeInSamples, maxSamples);
    } else {
        samplesToShow = maxSamples; // 0 означает "все время"
    }

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

void SegyViewer::mousePressEvent(QMouseEvent* event) {
    if (!dataManager) return;

    if (event->button() == Qt::LeftButton) {
        // Начинаем выделение области для зума
        isZooming = true;
        zoomStart = event->pos();
        zoomEnd = event->pos();
        hasZoomSelection = false;
        
        // Немедленная перерисовка для показа начальной точки
        update();
        
        // Принудительная перерисовка для мгновенного отображения
        repaint();
        
        // Устанавливаем флаг для отслеживания мыши
        setMouseTracking(true);
        
        // Запускаем таймер для плавного обновления
        zoomUpdateTimer->start();
    } else if (event->button() == Qt::RightButton) {
        // Правый клик - сброс зума
        resetZoom();
    }
}

void SegyViewer::mouseReleaseEvent(QMouseEvent* event) {
    if (!dataManager || !isZooming) return;

    if (event->button() == Qt::LeftButton) {
        isZooming = false;
        zoomEnd = event->pos();
        
        // Проверяем, что выделена достаточно большая область
        QRect selectionRect = QRect(zoomStart, zoomEnd).normalized();
        if (selectionRect.width() > 10 && selectionRect.height() > 10) {
            hasZoomSelection = true;
            updateZoomFromSelection();
        }
        
        // Отключаем отслеживание мыши после завершения выделения
        setMouseTracking(false);
        
        // Останавливаем таймер
        zoomUpdateTimer->stop();
        
        update();
    }
}

void SegyViewer::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // Двойной клик левой кнопкой - сброс зума
        resetZoom();
    }
}

void SegyViewer::drawSelectionRect(QPainter& painter) {
    if (!isZooming && !hasZoomSelection) return;
    
    QRect selectionRect = QRect(zoomStart, zoomEnd).normalized();
    if (selectionRect.width() < 2 || selectionRect.height() < 2) return;
    
    // Рисуем прямоугольник выделения
    painter.setPen(QPen(Qt::blue, 2, Qt::DashLine));
    painter.setBrush(QBrush(QColor(0, 0, 255, 30))); // Полупрозрачный синий
    painter.drawRect(selectionRect);
}

void SegyViewer::updateZoomFromSelection() {
    if (!dataManager || !hasZoomSelection) return;
    
    // Сохраняем текущее состояние перед зумом
    if (!isZoomed) {
        originalStartTrace = startTraceIndex;
        originalStartSample = startSampleIndex;
        originalTracesPerPage = tracesPerPage;
        originalSamplesPerPage = samplesPerPage;
        isZoomed = true;
    }
    
    // Учитываем отступы для осей
    const int leftMargin = 80;
    const int bottomMargin = 80;
    const int rightMargin = 20;
    const int topMargin = 20;
    int width = this->width() - leftMargin - rightMargin;
    int height = this->height() - topMargin - bottomMargin;
    
    if (width <= 0 || height <= 0) return;
    
    // Нормализуем прямоугольник выделения
    QRect selectionRect = QRect(zoomStart, zoomEnd).normalized();
    
    // Корректируем координаты с учетом отступов
    QPoint adjustedStart = selectionRect.topLeft() - QPoint(leftMargin, topMargin);
    QPoint adjustedEnd = selectionRect.bottomRight() - QPoint(leftMargin, topMargin);
    
    // Ограничиваем координаты областью отображения
    adjustedStart.setX(std::max(0, std::min(adjustedStart.x(), width)));
    adjustedStart.setY(std::max(0, std::min(adjustedStart.y(), height)));
    adjustedEnd.setX(std::max(0, std::min(adjustedEnd.x(), width)));
    adjustedEnd.setY(std::max(0, std::min(adjustedEnd.y(), height)));
    
    // Получаем текущие данные
    auto traces = dataManager->getTracesRange(startTraceIndex, tracesPerPage);
    if (traces.empty()) return;
    
    int traceCount = traces.size();
    int maxSamples = 0;
    for (const auto& trace : traces) {
        maxSamples = std::max(maxSamples, static_cast<int>(trace.size()));
    }
    
    if (maxSamples == 0) return;
    
    // Вычисляем текущие масштабы
    double pixelWidth = static_cast<double>(width) / traceCount;
    double pixelHeight = static_cast<double>(height) / maxSamples;
    
    // Вычисляем новые границы
    int newStartTrace = static_cast<int>(adjustedStart.x() / pixelWidth);
    int newEndTrace = static_cast<int>(adjustedEnd.x() / pixelWidth);
    int newStartSample = static_cast<int>(adjustedStart.y() / pixelHeight);
    int newEndSample = static_cast<int>(adjustedEnd.y() / pixelHeight);
    
    // Ограничиваем границы
    newStartTrace = std::max(0, std::min(newStartTrace, traceCount - 1));
    newEndTrace = std::max(newStartTrace + 1, std::min(newEndTrace, traceCount));
    newStartSample = std::max(0, std::min(newStartSample, maxSamples - 1));
    newEndSample = std::max(newStartSample + 1, std::min(newEndSample, maxSamples));
    
    // Применяем зум
    int newTracesPerPage = newEndTrace - newStartTrace;
    int newSamplesPerPage = newEndSample - newStartSample;
    
    // Обновляем параметры отображения
    startTraceIndex += newStartTrace;
    startSampleIndex += newStartSample;
    tracesPerPage = newTracesPerPage;
    samplesPerPage = newSamplesPerPage;
    
    // Сбрасываем выделение
    hasZoomSelection = false;
    isZooming = false;
    
    // Обновляем отображение
    colorMapValid = false;
    update();
    
    // Уведомляем об изменении зума
    emit zoomChanged();
}

void SegyViewer::resetZoom() {
    if (!isZoomed) return;
    
    // Восстанавливаем исходное состояние
    startTraceIndex = originalStartTrace;
    startSampleIndex = originalStartSample;
    tracesPerPage = originalTracesPerPage;
    samplesPerPage = originalSamplesPerPage;
    
    // Сбрасываем состояние зума
    isZoomed = false;
    isZooming = false;
    hasZoomSelection = false;
    
    // Останавливаем таймер
    zoomUpdateTimer->stop();
    
    // Обновляем отображение
    colorMapValid = false;
    update();
    
    // Уведомляем об изменении зума
    emit zoomChanged();
}

void SegyViewer::resetZoomTimeOnly() {
    if (!isZoomed) return;
    
    // Восстанавливаем только параметры времени
    startSampleIndex = originalStartSample;
    samplesPerPage = originalSamplesPerPage;
    
    // Обновляем отображение
    colorMapValid = false;
    update();
    
    // Уведомляем об изменении зума
    emit zoomChanged();
}

void SegyViewer::resetZoomTracesOnly() {
    if (!isZoomed) return;
    
    // Восстанавливаем только параметры трасс
    startTraceIndex = originalStartTrace;
    tracesPerPage = originalTracesPerPage;
    
    // Обновляем отображение
    colorMapValid = false;
    update();
    
    // Уведомляем об изменении зума
    emit zoomChanged();
}

void SegyViewer::zoomToRegion(int startTrace, int endTrace, int startSample, int endSample) {
    if (!dataManager) return;
    
    // Сохраняем текущее состояние перед зумом
    if (!isZoomed) {
        originalStartTrace = startTraceIndex;
        originalStartSample = startSampleIndex;
        originalTracesPerPage = tracesPerPage;
        originalSamplesPerPage = samplesPerPage;
        isZoomed = true;
    }
    
    // Применяем новый зум
    startTraceIndex = startTrace;
    startSampleIndex = startSample;
    tracesPerPage = endTrace - startTrace;
    samplesPerPage = endSample - startSample;
    
    // Сбрасываем выделение
    hasZoomSelection = false;
    isZooming = false;
    
    // Обновляем отображение
    colorMapValid = false;
    update();
    
    // Уведомляем об изменении зума
    emit zoomChanged();
}

QString SegyViewer::getZoomHelpText() const {
    return QString(
        "Zoom Controls:\n"
        "• Left mouse button + drag: Select area to zoom\n"
        "• Right mouse button: Reset zoom\n"
        "• Double left click: Reset zoom\n"
        "• Menu: View → Reset Zoom"
    );
}
