#pragma once

#include <QMainWindow> // обязательно
#include <QScrollBar>
#include "SegyViewer.hpp"
#include "SegyDataManager.hpp"
#include "StatusPanel.hpp"
#include "SettingsPanel.hpp"
#include "TraceInfoPanel.hpp"

struct ViewerSettings; // или #include "ViewerSettings.hpp", если есть файл

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openAsTraces();
    void openAsGathers();
    void openSettings();
    void traceUnderCursor(int traceIndex, int sampleIndex, float amplitude);
    void onSettingsChanged();
    void onScrollBarChanged(int value);
    void onVerticalScrollBarChanged(int value);
    
    // Новые слоты для продвинутых настроек цветовых схем
    void openGammaDialog();
    void openContrastDialog();
    void togglePerceptualCorrection(bool enabled);
    void resetColorSettings();
    
    // Слоты для обработки изменений слайдеров
    void onContrastSliderChanged(int value);
    void onBrightnessSliderChanged(int value);

private:
    void createMenus();
    void setupScrollBar();
    
    SegyViewer* viewer;
    SegyDataManager* dataManager;
    StatusPanel* statusPanel;
    SettingsPanel* settingsPanel;
    TraceInfoPanel* traceInfoPanel;
    QScrollBar* scrollBar;
    QScrollBar* verticalScrollBar; // Вертикальный скролл-бар для сэмплов
    
    // Навигация
    int navigationStep;
    float currentGain; // Для сохранения значения gain
    
    // Продвинутые настройки цветовых схем
    float currentGamma;
    float currentContrast;
    float currentBrightness;
    bool currentPerceptualCorrection;
    
    // Ссылки на действия меню для обновления состояния
    QAction* perceptualAction;
    
    // Ссылки на слайдеры для обновления настроек
    QSlider* contrastSlider;
    QSlider* brightnessSlider;
};

