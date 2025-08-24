#include "MainWindow.hpp"
#include "SegyViewer.hpp"
#include "SegyDataManager.hpp"
#include "StatusPanel.hpp"
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      viewer(new SegyViewer(this)),
      dataManager(new SegyDataManager(1000)), // Кэш на 1000 трасс
      statusPanel(new StatusPanel(this)),
      settingsPanel(new SettingsPanel(this)),
      traceInfoPanel(new TraceInfoPanel(this)),
      scrollBar(new QScrollBar(Qt::Horizontal, this)),
      navigationStep(10),
      currentGain(5.0f)
{
    setWindowTitle("SEG-Y Viewer");
    resize(1500, 1050); // 1.5 раза больше

    QWidget *central = new QWidget(this);
    
    // Создаем горизонтальный layout для основного содержимого
    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(5);
    
    // Левая часть с графиком и настройками
    QWidget *leftWidget = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0,0,0,0);
    leftLayout->setSpacing(2);
    leftLayout->addWidget(settingsPanel); // Панель настроек сверху (фиксированная высота)
    leftLayout->addWidget(viewer, 1); // График занимает всю доступную высоту
    leftLayout->addWidget(scrollBar); // Скролл-бар для навигации
    leftLayout->addWidget(statusPanel); // Статусная панель (фиксированная высота)
    
    // Добавляем левую часть и панель с информацией о трассе
    mainLayout->addWidget(leftWidget, 7); // 70% для основного содержимого
    mainLayout->addWidget(traceInfoPanel, 3); // 30% для панели с информацией
    
    central->setLayout(mainLayout);
    setCentralWidget(central);

    createMenus();
    setupScrollBar();

    // Подключаем сигналы от панели настроек
    connect(settingsPanel, &SettingsPanel::settingsChanged, this, &MainWindow::onSettingsChanged);
    
    // Инициализируем панель настроек
    settingsPanel->setTracesPerPage(200);
    settingsPanel->setNavigationStep(10);
    settingsPanel->setColorScheme("Grayscale");
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

    QMenu* viewMenu = menuBar()->addMenu("&View");
    // Настройки теперь в панели над графиком
}

void MainWindow::setupScrollBar() {
    scrollBar->setVisible(false); // Скрыт по умолчанию
    scrollBar->setMinimum(0);
    scrollBar->setMaximum(0);
    scrollBar->setValue(0);
    scrollBar->setPageStep(1);
    scrollBar->setSingleStep(navigationStep); // Синхронизируем с Navigation Step
    
    connect(scrollBar, &QScrollBar::valueChanged, this, &MainWindow::onScrollBarChanged);
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
        return;
    }

    viewer->setDataManager(dataManager);
    viewer->setCurrentPage(0);
    viewer->setGain(currentGain); // Применяем gain сразу при загрузке
    
    // Настраиваем скролл-бар
    int totalTraces = dataManager->traceCount();
    int tracesPerPage = settingsPanel->getTracesPerPage();
    int navigationStep = settingsPanel->getNavigationStep();
    int maxValue = std::max(0, totalTraces - tracesPerPage);
    
    scrollBar->setVisible(true);
    scrollBar->setMinimum(0);
    scrollBar->setMaximum(maxValue);
    scrollBar->setValue(0);
    scrollBar->setPageStep(tracesPerPage);
    scrollBar->setSingleStep(navigationStep); // Синхронизируем с Navigation Step
    
    viewer->update();
}

void MainWindow::onScrollBarChanged(int value) {
    // Устанавливаем начальную трассу напрямую
    viewer->setStartTrace(value);
}

void MainWindow::traceUnderCursor(int traceIndex, int sampleIndex, float amplitude) {
    statusPanel->updateInfo(traceIndex, sampleIndex, amplitude);
    
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
    viewer->setTracesPerPage(settingsPanel->getTracesPerPage());
    viewer->setColorScheme(settingsPanel->getColorScheme());
    viewer->setGain(settingsPanel->getGain());

    
    // Обновляем размер кэша
    dataManager->setCacheSize(settingsPanel->getCacheSize());
    
    // Обновляем gain
    currentGain = settingsPanel->getGain();
    
    // Обновляем скролл-бар если файл загружен
    if (dataManager && dataManager->traceCount() > 0) {
        int totalTraces = dataManager->traceCount();
        int tracesPerPage = settingsPanel->getTracesPerPage();
        int navigationStep = settingsPanel->getNavigationStep();
        int maxValue = std::max(0, totalTraces - tracesPerPage);
        
        scrollBar->setMaximum(maxValue);
        scrollBar->setPageStep(tracesPerPage);
        scrollBar->setSingleStep(navigationStep); // Обновляем SingleStep при изменении Navigation Step
        
        // Убеждаемся, что текущее значение не превышает максимум
        if (scrollBar->value() > maxValue) {
            scrollBar->setValue(maxValue);
        }
    }
    
    // Обновляем отображение
    viewer->update();
}

