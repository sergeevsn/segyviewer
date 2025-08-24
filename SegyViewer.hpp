#pragma once
#include <QWidget>
#include <vector>
#include <limits>

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
    void setColorScheme(const QString& scheme);
    void setGain(float g) { gain = g; colorMapValid = false; }

signals:
    void traceInfoUnderCursor(int traceIndex, int sampleIndex, float amplitude);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void updateColorMap();
    QColor amplitudeToColor(float amplitude) const;
    
    SegyDataManager* dataManager;
    int pageIndex;
    int startTraceIndex; // Начальный индекс трассы для отображения
    int tracesPerPage;
    QString colorScheme;
    
    // Для цветовой карты
    float minAmplitude;
    float maxAmplitude;
    bool colorMapValid;
    float gain;
    bool globalStatsComputed;
};

