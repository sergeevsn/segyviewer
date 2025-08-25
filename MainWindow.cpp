#include "MainWindow.hpp"
#include "SegyViewer.hpp"
#include "SegyDataManager.hpp"
#include "StatusPanel.hpp"
#include "SettingsPanel.hpp"
#include "TraceInfoPanel.hpp"
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QAction>
#include <QMenu>
#include <QCoreApplication>
#include <iostream>
#include <QInputDialog>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QDialog>
#include <QHBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      viewer(new SegyViewer(this)),
      dataManager(new SegyDataManager(1000)), // Кэш на 1000 трасс
      statusPanel(new StatusPanel(this)),
      settingsPanel(new SettingsPanel(this)),
      traceInfoPanel(new TraceInfoPanel(this)),
          scrollBar(new QScrollBar(Qt::Horizontal, this)),
    verticalScrollBar(new QScrollBar(Qt::Vertical, this)),
    navigationStep(10),
    currentGain(1.0f),
    currentGamma(2.2f),
    currentContrast(1.0f),
    currentBrightness(0.0f),
    currentPerceptualCorrection(false)
{
    setWindowTitle("SEG-Y Viewer");
    resize(1600, 1000); // Увеличенный размер для лучшего отображения данных

    QWidget *central = new QWidget(this);
    
    // Создаем горизонтальный layout для основного содержимого
    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(5);
    
    // Левая часть с графиком и настройками
    QWidget *leftWidget = new QWidget(this);
    QHBoxLayout *leftLayout = new QHBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0,0,0,0);
    leftLayout->setSpacing(2);
    
    // Центральная часть с настройками, графиком и горизонтальным скролл-баром
    QVBoxLayout *centerLayout = new QVBoxLayout();
    centerLayout->setContentsMargins(0,0,0,0);
    centerLayout->setSpacing(2);
    centerLayout->addWidget(settingsPanel); // Панель настроек сверху (фиксированная высота)
    centerLayout->addWidget(viewer, 1); // График занимает всю доступную высоту
    centerLayout->addWidget(scrollBar); // Скролл-бар для навигации
    centerLayout->addWidget(statusPanel); // Статусная панель (фиксированная высота)
    
    leftLayout->addLayout(centerLayout, 1);
    leftLayout->addWidget(verticalScrollBar); // Вертикальный скролл-бар справа
    
    // Устанавливаем фиксированную ширину для вертикального скролл-бара
    verticalScrollBar->setFixedWidth(20);
    
    // Добавляем левую часть и панель с информацией о трассе
    mainLayout->addWidget(leftWidget, 8); // 80% для основного содержимого
    mainLayout->addWidget(traceInfoPanel, 2); // 20% для панели с информацией
    
    central->setLayout(mainLayout);
    setCentralWidget(central);

    createMenus();
    setupScrollBar();

    // Подключаем сигналы от панели настроек
    connect(settingsPanel, &SettingsPanel::settingsChanged, this, &MainWindow::onSettingsChanged);
    
    // Инициализируем панель настроек
    settingsPanel->setTracesPerPage(1000);
    settingsPanel->setSamplesPerPage(0);
    settingsPanel->setColorScheme("gray");
    settingsPanel->setGain(currentGain);

    connect(viewer, &SegyViewer::traceInfoUnderCursor,
            this, &MainWindow::traceUnderCursor);
}

void MainWindow::createMenus() {
    QMenu* fileMenu = menuBar()->addMenu("&File");

    QAction* openTracesAct = new QAction("Open as Traces", this);
    connect(openTracesAct, &QAction::triggered, this, &MainWindow::openAsTraces);
    fileMenu->addAction(openTracesAct);

    fileMenu->addSeparator();
    fileMenu->addAction("Exit", qApp, &QCoreApplication::quit);

    // Меню View с продвинутыми настройками цветовых схем
    QMenu* viewMenu = menuBar()->addMenu("&View");
    
    // Подменю для цветовых схем
    QMenu* colorMenu = viewMenu->addMenu("Color Scheme Settings");
    
    // Gamma correction
    QAction* gammaAction = new QAction("Gamma Correction...", this);
    connect(gammaAction, &QAction::triggered, this, &MainWindow::openGammaDialog);
    colorMenu->addAction(gammaAction);
    
    // Contrast and brightness
    QAction* contrastAction = new QAction("Contrast & Brightness...", this);
    connect(contrastAction, &QAction::triggered, this, &MainWindow::openContrastDialog);
    colorMenu->addAction(contrastAction);
    
    // Perceptual correction
    perceptualAction = new QAction("Perceptual Correction", this);
    perceptualAction->setCheckable(true);
    perceptualAction->setChecked(false);
    connect(perceptualAction, &QAction::toggled, this, &MainWindow::togglePerceptualCorrection);
    colorMenu->addAction(perceptualAction);
    
    // Reset all settings
    colorMenu->addSeparator();
    QAction* resetAction = new QAction("Reset All Settings", this);
    connect(resetAction, &QAction::triggered, this, &MainWindow::resetColorSettings);
    colorMenu->addAction(resetAction);
}

void MainWindow::setupScrollBar() {
    scrollBar->setVisible(false); // Скрыт по умолчанию
    scrollBar->setMinimum(0);
    scrollBar->setMaximum(0);
    scrollBar->setValue(0);
    scrollBar->setPageStep(1);
    scrollBar->setSingleStep(navigationStep); // Синхронизируем с Navigation Step
    
    connect(scrollBar, &QScrollBar::valueChanged, this, &MainWindow::onScrollBarChanged);
    
    // Настройка вертикального скролл-бара для сэмплов
    verticalScrollBar->setVisible(false); // Скрыт по умолчанию
    verticalScrollBar->setMinimum(0);
    verticalScrollBar->setMaximum(0);
    verticalScrollBar->setValue(0);
    verticalScrollBar->setPageStep(1);
    verticalScrollBar->setSingleStep(1);
    
    connect(verticalScrollBar, &QScrollBar::valueChanged, this, &MainWindow::onVerticalScrollBarChanged);
}

void MainWindow::openSettings() {
    // Настройки теперь в панели над графиком
    // Этот метод оставлен для совместимости, но не используется
}

void MainWindow::openAsTraces() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open SEG-Y File", "", "SEG-Y Files (*.sgy *.segy)");
    if (fileName.isEmpty())
        return;

    if (!dataManager->loadFile(fileName.toStdString())) {
        QMessageBox::warning(this, "Error", "Failed to load SEG-Y file");
        // Сбрасываем информацию о файле в SettingsPanel
        settingsPanel->setFileInfo(0, 0.0f);
        return;
    }

    viewer->setDataManager(dataManager);
    viewer->setCurrentPage(0);
    viewer->setGain(currentGain); // Применяем gain сразу при загрузке
    viewer->setGridEnabled(settingsPanel->getGridEnabled()); // Применяем настройку сетки
    
    // Обновляем информацию о файле в SettingsPanel
    int totalSamples = 0;
    float dt = 0.0f;
    if (dataManager->traceCount() > 0) {
        auto traces = dataManager->getTracesRange(0, 1);
        if (!traces.empty() && !traces[0].empty()) {
            totalSamples = traces[0].size();
            dt = dataManager->getSampleInterval();
        }
    }
    settingsPanel->setFileInfo(totalSamples, dt);
    
    // Автоматически устанавливаем time per page равным полному времени трассы
    if (totalSamples > 0) {
        float dt = dataManager->getSampleInterval(); // в микросекундах
        float totalTimeMs = (totalSamples - 1) * dt; // общее время в миллисекундах (сэмплы от 0 до totalSamples-1)
        int timePerPageMs = static_cast<int>(totalTimeMs);
        
        settingsPanel->setSamplesPerPage(timePerPageMs);
        viewer->setSamplesPerPage(timePerPageMs);
    }
    
    // Настраиваем горизонтальный скролл-бар
    int totalTraces = dataManager->traceCount();
    int tracesPerPage = settingsPanel->getTracesPerPage();
    
    // Если tracesPerPage = 0, используем максимальное количество трасс с ограничением 5000
    if (tracesPerPage == 0) {
        tracesPerPage = std::min(totalTraces, 5000);
        
        // Обновляем значение в spin box, чтобы показать реальное установленное значение
        settingsPanel->setTracesPerPage(tracesPerPage);
    }
    
    // Теперь устанавливаем вычисленное значение в viewer
    viewer->setTracesPerPage(tracesPerPage);
    
    int maxValue = std::max(0, totalTraces - tracesPerPage);
    
    scrollBar->setVisible(true);
    scrollBar->setMinimum(0);
    scrollBar->setMaximum(maxValue);
    scrollBar->setValue(0);
    scrollBar->setPageStep(tracesPerPage);
    scrollBar->setSingleStep(10); // Фиксированный шаг навигации
    
    // Настраиваем вертикальный скролл-бар для сэмплов
    if (totalSamples > 0) {
        // Получаем установленное значение time per page
        int timePerPage = settingsPanel->getSamplesPerPage();
        
        // Конвертируем время в количество сэмплов для сравнения
        float dt = dataManager->getSampleInterval(); // в микросекундах
        int timeInSamples = static_cast<int>(timePerPage / dt);
        
        if (timeInSamples > 0 && timeInSamples < totalSamples) {
            // Если time per page меньше полного времени, показываем скролл-бар
            int maxSampleValue = std::max(0, totalSamples - timeInSamples);
            
            verticalScrollBar->setVisible(true);
            verticalScrollBar->setMinimum(0);
            verticalScrollBar->setMaximum(maxSampleValue);
            verticalScrollBar->setValue(0);
            verticalScrollBar->setPageStep(timeInSamples);
        } else {
            // Если показываем все время, скрываем скролл-бар
            verticalScrollBar->setVisible(false);
            verticalScrollBar->setMinimum(0);
            verticalScrollBar->setMaximum(0);
            verticalScrollBar->setValue(0);
            verticalScrollBar->setPageStep(totalSamples);
        }
        
        // Сбрасываем начальный сэмпл в viewer
        viewer->setStartSample(0);
    } else {
        // Если нет сэмплов, скрываем вертикальный скролл-бар
        verticalScrollBar->setVisible(false);
        viewer->setStartSample(0);
    }
    
    viewer->update();
}

void MainWindow::onScrollBarChanged(int value) {
    // Устанавливаем начальную трассу напрямую
    viewer->setStartTrace(value);
}

void MainWindow::onVerticalScrollBarChanged(int value) {
    // Устанавливаем начальный сэмпл напрямую
    viewer->setStartSample(value);
    // Обновляем отображение
    viewer->update();
}

void MainWindow::traceUnderCursor(int traceIndex, int sampleIndex, float amplitude) {
    float dt = dataManager ? dataManager->getSampleInterval() : 0.0f;
    statusPanel->updateInfo(traceIndex, sampleIndex, amplitude, dt);
    
    // Обновляем информацию о заголовке трассы
    if (dataManager) {
        std::vector<uint8_t> traceHeader = dataManager->getTraceHeader(traceIndex);
        traceInfoPanel->updateTraceInfo(traceIndex, traceHeader);
    }
}

void MainWindow::openAsGathers() {
    // Заглушка для метода открытия как gathers
    QMessageBox::information(this, "Info", "Gathers functionality not implemented yet");
}

MainWindow::~MainWindow() {
    // Деструктор
}

void MainWindow::onSettingsChanged() {
    // Применяем новые настройки к viewer
    int tracesPerPage = settingsPanel->getTracesPerPage();
    
    // Если tracesPerPage = 0, используем максимальное количество трасс с ограничением 5000
    if (dataManager && tracesPerPage == 0) {
        int totalTraces = dataManager->traceCount();
        tracesPerPage = std::min(totalTraces, 5000);
        
        // Обновляем значение в spin box, чтобы показать реальное установленное значение
        settingsPanel->setTracesPerPage(tracesPerPage);
    }
    
    viewer->setTracesPerPage(tracesPerPage);
    viewer->setSamplesPerPage(settingsPanel->getSamplesPerPage());
    viewer->setColorScheme(settingsPanel->getColorScheme());
    viewer->setGain(settingsPanel->getGain());
    viewer->setGridEnabled(settingsPanel->getGridEnabled());
    
    // Устанавливаем фиксированный размер кэша (5000)
    dataManager->setCacheSize(5000);
    
    // Обновляем gain
    currentGain = settingsPanel->getGain();
    
    // Обновляем горизонтальный скролл-бар если файл загружен
    if (dataManager && dataManager->traceCount() > 0) {
        int totalTraces = dataManager->traceCount();
        int tracesPerPage = settingsPanel->getTracesPerPage();
        
        // Если tracesPerPage = 0, используем максимальное количество трасс с ограничением 5000
        if (tracesPerPage == 0) {
            tracesPerPage = std::min(totalTraces, 5000);
        }
        
        int maxValue = std::max(0, totalTraces - tracesPerPage);
        
        scrollBar->setMaximum(maxValue);
        scrollBar->setPageStep(tracesPerPage);
        scrollBar->setSingleStep(10); // Фиксированный шаг навигации
        
        // Убеждаемся, что текущее значение не превышает максимум
        if (scrollBar->value() > maxValue) {
            scrollBar->setValue(maxValue);
        }
    }
    
    // Обновляем вертикальный скролл-бар для сэмплов
    if (dataManager && dataManager->traceCount() > 0) {
        int samplesPerPage = settingsPanel->getSamplesPerPage();
        // Получаем количество сэмплов в первой трассе для оценки
        auto traces = dataManager->getTracesRange(0, 1);
        if (!traces.empty() && !traces[0].empty()) {
            int totalSamples = traces[0].size();
            
            // Если samplesPerPage = 0, вычисляем количество сэмплов для полного времени
            if (samplesPerPage == 0) {
                float dt = dataManager->getSampleInterval(); // в микросекундах
                int totalTimeMs = (totalSamples - 1) * dt; // общее время в миллисекундах (сэмплы от 0 до totalSamples-1)
                samplesPerPage = totalTimeMs;
                
                // Обновляем значение в spin box, чтобы показать реальное установленное значение
                settingsPanel->setSamplesPerPage(samplesPerPage);
            }
            
            if (samplesPerPage > 0 && samplesPerPage < totalSamples) {
                // Если samples per page меньше общего количества сэмплов, показываем скролл-бар
                int maxSampleValue = std::max(0, totalSamples - samplesPerPage);
                
                verticalScrollBar->setVisible(true);
                verticalScrollBar->setMinimum(0);
                verticalScrollBar->setMaximum(maxSampleValue);
                verticalScrollBar->setValue(0);
                verticalScrollBar->setPageStep(samplesPerPage);
                
                // Сбрасываем начальный сэмпл в viewer
                viewer->setStartSample(0);
            } else {
                // Если samples per page = 0 или >= totalSamples, скрываем вертикальный скролл-бар
                verticalScrollBar->setVisible(false);
                viewer->setStartSample(0);
            }
        }
    }
    
    // Обновляем отображение
    viewer->update();
}

// Реализация методов для продвинутых настроек цветовых схем
void MainWindow::openGammaDialog() {
    bool ok;
    float gamma = QInputDialog::getDouble(this, "Gamma Correction", 
                                        "Enter gamma value (0.5 - 4.0):", 
                                        currentGamma, 0.5, 4.0, 1, &ok);
    if (ok) {
        currentGamma = gamma;
        viewer->setGamma(gamma);
        viewer->update();
    }
}

void MainWindow::openContrastDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle("Contrast & Brightness");
    dialog.setModal(true);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    // Contrast slider
    QHBoxLayout* contrastLayout = new QHBoxLayout();
    QLabel* contrastLabel = new QLabel("Contrast:", &dialog);
    QSlider* contrastSlider = new QSlider(Qt::Horizontal, &dialog);
    contrastSlider->setRange(10, 300); // 0.1 - 3.0 * 100
    contrastSlider->setValue(static_cast<int>(currentContrast * 100));
    contrastLayout->addWidget(contrastLabel);
    contrastLayout->addWidget(contrastSlider);
    layout->addLayout(contrastLayout);
    
    // Brightness slider
    QHBoxLayout* brightnessLayout = new QHBoxLayout();
    QLabel* brightnessLabel = new QLabel("Brightness:", &dialog);
    QSlider* brightnessSlider = new QSlider(Qt::Horizontal, &dialog);
    brightnessSlider->setRange(-50, 50); // -0.5 - 0.5 * 100
    brightnessSlider->setValue(static_cast<int>(currentBrightness * 100));
    brightnessLayout->addWidget(brightnessLabel);
    brightnessLayout->addWidget(brightnessSlider);
    layout->addLayout(brightnessLayout);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okButton = new QPushButton("OK", &dialog);
    QPushButton* cancelButton = new QPushButton("Cancel", &dialog);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);
    
    // Connect signals
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    // Apply changes in real-time
    connect(contrastSlider, &QSlider::valueChanged, [this, contrastSlider, brightnessSlider]() {
        float contrast = contrastSlider->value() / 100.0f;
        float brightness = brightnessSlider->value() / 100.0f;
        currentContrast = contrast;
        currentBrightness = brightness;
        viewer->setContrast(contrast);
        viewer->setBrightness(brightness);
        viewer->update();
    });
    
    connect(brightnessSlider, &QSlider::valueChanged, [this, contrastSlider, brightnessSlider]() {
        float contrast = contrastSlider->value() / 100.0f;
        float brightness = brightnessSlider->value() / 100.0f;
        currentContrast = contrast;
        currentBrightness = brightness;
        viewer->setContrast(contrast);
        viewer->setBrightness(brightness);
        viewer->update();
    });
    
    dialog.exec();
}

void MainWindow::togglePerceptualCorrection(bool enabled) {
    currentPerceptualCorrection = enabled;
    viewer->setPerceptualCorrection(enabled);
    viewer->update();
}

void MainWindow::resetColorSettings() {
    // Сбрасываем все настройки к значениям по умолчанию
    currentGamma = 2.2f;
    currentContrast = 1.0f;
    currentBrightness = 0.0f;
    currentPerceptualCorrection = false;
    
    // Применяем сброшенные настройки к viewer
    viewer->setGamma(currentGamma);
    viewer->setContrast(currentContrast);
    viewer->setBrightness(currentBrightness);
    viewer->setPerceptualCorrection(currentPerceptualCorrection);
    
    // Обновляем состояние чекбокса в меню
    perceptualAction->setChecked(false);
    
    // Обновляем отображение
    viewer->update();
    
    // Показываем уведомление пользователю
    QMessageBox::information(this, "Settings Reset", 
                           "All color scheme settings have been reset to default values:\n"
                           "• Gamma: 2.2\n"
                           "• Contrast: 1.0\n"
                           "• Brightness: 0.0\n"
                           "• Perceptual Correction: Off");
}

