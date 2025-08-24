#include "SettingsPanel.hpp"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>

SettingsPanel::SettingsPanel(QWidget* parent)
    : QWidget(parent)
{
    // Создаем горизонтальный layout для групп
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(10);
    
    // Группа 1: Display Settings
    QGroupBox* displayGroup = new QGroupBox("", this);
    QVBoxLayout* displayLayout = new QVBoxLayout(displayGroup);
    
    // Traces per page
    QHBoxLayout* tracesLayout = new QHBoxLayout();
    QLabel* tracesLabel = new QLabel("Traces per page:", this);
    tracesSpinBox = new QSpinBox(this);
    tracesSpinBox->setRange(10, 5000);
    tracesSpinBox->setValue(1000);
    tracesLayout->addWidget(tracesLabel);
    tracesLayout->addWidget(tracesSpinBox);
    displayLayout->addLayout(tracesLayout);
    
    mainLayout->addWidget(displayGroup);
    
    // Группа 2: Trace Step Settings
    QGroupBox* stepGroup = new QGroupBox("", this);
    QVBoxLayout* stepLayout = new QVBoxLayout(stepGroup);
    
    // Trace Step
    QHBoxLayout* stepControlLayout = new QHBoxLayout();
    QLabel* stepLabel = new QLabel("Trace Step:", this);
    stepSpinBox = new QSpinBox(this);
    stepSpinBox->setRange(1, 100);
    stepSpinBox->setValue(10);
    stepControlLayout->addWidget(stepLabel);
    stepControlLayout->addWidget(stepSpinBox);
    stepLayout->addLayout(stepControlLayout);
    
    mainLayout->addWidget(stepGroup);
    
    // Группа 3: Samples Per Page Settings
    QGroupBox* samplesGroup = new QGroupBox("", this);
    QVBoxLayout* samplesLayout = new QVBoxLayout(samplesGroup);
    
    // Samples Per Page
    QHBoxLayout* samplesControlLayout = new QHBoxLayout();
    QLabel* samplesLabel = new QLabel("Samples per page:", this);
    samplesSpinBox = new QSpinBox(this);
    samplesSpinBox->setRange(0, 10000);
    samplesSpinBox->setValue(0);
    samplesSpinBox->setSpecialValueText("All"); // 0 означает "все сэмплы"
    samplesControlLayout->addWidget(samplesLabel);
    samplesControlLayout->addWidget(samplesSpinBox);
    samplesLayout->addLayout(samplesControlLayout);
    
    mainLayout->addWidget(samplesGroup);
    
    // Группа 4: Color Settings
    QGroupBox* colorGroup = new QGroupBox("", this);
    QVBoxLayout* colorLayout = new QVBoxLayout(colorGroup);
    
    // Color scheme
    QHBoxLayout* schemeLayout = new QHBoxLayout();
    QLabel* colorLabel = new QLabel("Color scheme:", this);
    colorCombo = new QComboBox(this);
    colorCombo->addItems({"gray", "PuOr", "seismic"});
    colorCombo->setCurrentText("gray");
    schemeLayout->addWidget(colorLabel);
    schemeLayout->addWidget(colorCombo);
    colorLayout->addLayout(schemeLayout);
    
    mainLayout->addWidget(colorGroup);
    
    // Группа 5: Image Settings
    QGroupBox* imageGroup = new QGroupBox("", this);
    QVBoxLayout* imageLayout = new QVBoxLayout(imageGroup);
    
    // Gain
    QHBoxLayout* gainLayout = new QHBoxLayout();
    QLabel* gainLabel = new QLabel("Gain:", this);
    gainSpinBox = new QDoubleSpinBox(this);
    gainSpinBox->setRange(0.1, 50.0);
    gainSpinBox->setSingleStep(0.1);
    gainSpinBox->setDecimals(1);
    gainSpinBox->setValue(5.0);
    gainLayout->addWidget(gainLabel);
    gainLayout->addWidget(gainSpinBox);
    imageLayout->addLayout(gainLayout);
    
    mainLayout->addWidget(imageGroup);
    
    // Группа 6: Cache Settings
    QGroupBox* cacheGroup = new QGroupBox("", this);
    QVBoxLayout* cacheLayout = new QVBoxLayout(cacheGroup);
    
    // Cache size
    QHBoxLayout* cacheSizeLayout = new QHBoxLayout();
    QLabel* cacheLabel = new QLabel("Cache size:", this);
    cacheSpinBox = new QSpinBox(this);
    cacheSpinBox->setRange(100, 10000);
    cacheSpinBox->setValue(1000);
    cacheSizeLayout->addWidget(cacheLabel);
    cacheSizeLayout->addWidget(cacheSpinBox);
    cacheLayout->addLayout(cacheSizeLayout);
    
    mainLayout->addWidget(cacheGroup);
    
    // Добавляем растягивающийся элемент в конец
    mainLayout->addStretch();
    
    // Подключаем сигналы
    connect(tracesSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onTracesPerPageChanged()));
    connect(stepSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onNavigationStepChanged()));
    connect(samplesSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onSamplesPerPageChanged()));
    connect(colorCombo, SIGNAL(currentTextChanged(QString)), this, SLOT(onColorSchemeChanged()));
    connect(gainSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onGainChanged()));
    connect(cacheSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onCacheSizeChanged()));
}

// Геттеры
int SettingsPanel::getTracesPerPage() const {
    return tracesSpinBox->value();
}

int SettingsPanel::getNavigationStep() const {
    return stepSpinBox->value();
}

int SettingsPanel::getSamplesPerPage() const {
    return samplesSpinBox->value();
}

QString SettingsPanel::getColorScheme() const {
    return colorCombo->currentText();
}

float SettingsPanel::getGain() const {
    return static_cast<float>(gainSpinBox->value());
}

int SettingsPanel::getCacheSize() const {
    return cacheSpinBox->value();
}

// Сеттеры
void SettingsPanel::setTracesPerPage(int value) {
    tracesSpinBox->setValue(value);
}

void SettingsPanel::setNavigationStep(int value) {
    stepSpinBox->setValue(value);
}

void SettingsPanel::setSamplesPerPage(int value) {
    samplesSpinBox->setValue(value);
}

void SettingsPanel::setColorScheme(const QString& scheme) {
    colorCombo->setCurrentText(scheme);
}

void SettingsPanel::setGain(float value) {
    gainSpinBox->setValue(value);
}

void SettingsPanel::setCacheSize(int value) {
    cacheSpinBox->setValue(value);
}

// Слоты для синхронизации
void SettingsPanel::onTracesPerPageChanged() {
    emit settingsChanged();
}

void SettingsPanel::onNavigationStepChanged() {
    emit settingsChanged();
}

void SettingsPanel::onSamplesPerPageChanged() {
    emit settingsChanged();
}

void SettingsPanel::onColorSchemeChanged() {
    emit settingsChanged();
}

void SettingsPanel::onGainChanged() {
    emit settingsChanged();
}

void SettingsPanel::onCacheSizeChanged() {
    emit settingsChanged();
}
