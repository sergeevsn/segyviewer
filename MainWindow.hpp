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

private:
    void createMenus();
    void setupScrollBar();
    
    SegyViewer* viewer;
    SegyDataManager* dataManager;
    StatusPanel* statusPanel;
    SettingsPanel* settingsPanel;
    TraceInfoPanel* traceInfoPanel;
    QScrollBar* scrollBar;
    
    // Навигация
    int navigationStep;
    float currentGain; // Для сохранения значения gain
};

