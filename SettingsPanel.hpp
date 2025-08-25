#pragma once

#include <QWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTimer>

struct ViewerSettings;

class SettingsPanel : public QWidget {
    Q_OBJECT

public:
    explicit SettingsPanel(QWidget* parent = nullptr);
    
    // Геттеры для получения текущих значений
    int getTracesPerPage() const;
    int getSamplesPerPage() const;
    QString getColorScheme() const;
    float getGain() const;
    bool getGridEnabled() const;
    
    // Сеттеры для установки значений
    void setTracesPerPage(int value);
    void setSamplesPerPage(int value);
    void setColorScheme(const QString& scheme);
    void setGain(float value);
    void setGridEnabled(bool enabled);
    
    // Методы для отображения информации о файле
    void setFileInfo(int samples, float dt);

signals:
    void settingsChanged(); // Сигнал при изменении любых настроек

private slots:
    void onTracesPerPageChanged();
    void onSamplesPerPageChanged();
    void onSamplesPerPageValueChanged();
    void onSamplesPerPageDebounced();
    void onColorSchemeChanged();
    void onGainChanged();
    void onGridEnabledChanged();

private:
    // Виджеты для traces per page
    QSpinBox* tracesSpinBox;
    
    // Виджеты для time per page
    QSpinBox* samplesSpinBox;
    
    // Виджет для color scheme
    QComboBox* colorCombo;
    
    // Виджеты для gain
    QDoubleSpinBox* gainSpinBox;
    
    // Виджет для включения/отключения сетки
    QCheckBox* gridCheckBox;
    
    // Виджеты для отображения информации о файле
    QLabel* samplesLabel;
    QLabel* dtLabel;
    
    // Таймер для дебаунсинга изменений samples per page
    QTimer* samplesDebounceTimer;
};


