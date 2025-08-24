#pragma once

#include <QWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QGroupBox>

struct ViewerSettings;

class SettingsPanel : public QWidget {
    Q_OBJECT

public:
    explicit SettingsPanel(QWidget* parent = nullptr);
    
    // Геттеры для получения текущих значений
    int getTracesPerPage() const;
    int getNavigationStep() const;
    int getSamplesPerPage() const;
    QString getColorScheme() const;
    float getGain() const;
    int getCacheSize() const;
    
    // Сеттеры для установки значений
    void setTracesPerPage(int value);
    void setNavigationStep(int value);
    void setSamplesPerPage(int value);
    void setColorScheme(const QString& scheme);
    void setGain(float value);
    void setCacheSize(int value);

signals:
    void settingsChanged(); // Сигнал при изменении любых настроек

private slots:
    void onTracesPerPageChanged();
    void onNavigationStepChanged();
    void onSamplesPerPageChanged();
    void onColorSchemeChanged();
    void onGainChanged();
    void onCacheSizeChanged();

private:
    // Виджеты для traces per page
    QSpinBox* tracesSpinBox;
    
    // Виджеты для navigation step
    QSpinBox* stepSpinBox;
    
    // Виджеты для samples per page
    QSpinBox* samplesSpinBox;
    
    // Виджет для color scheme
    QComboBox* colorCombo;
    
    // Виджеты для gain
    QDoubleSpinBox* gainSpinBox;
    
    // Виджеты для cache size
    QSpinBox* cacheSpinBox;
};


