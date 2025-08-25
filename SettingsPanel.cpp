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
    tracesSpinBox->setRange(0, 5000);
    tracesSpinBox->setValue(1000);
    tracesSpinBox->setSpecialValueText("Max"); // 0 означает "максимум трасс"
    tracesLayout->addWidget(tracesLabel);
    tracesLayout->addWidget(tracesSpinBox);
    displayLayout->addLayout(tracesLayout);
    
    mainLayout->addWidget(displayGroup);
    
    // Группа 2: Samples Per Page Settings
    QGroupBox* samplesGroup = new QGroupBox("", this);
    QVBoxLayout* samplesLayout = new QVBoxLayout(samplesGroup);
    
    // Time Per Page
    QHBoxLayout* samplesControlLayout = new QHBoxLayout();
    QLabel* samplesPerPageLabel = new QLabel("Time per page (ms):", this);
    samplesSpinBox = new QSpinBox(this);
    samplesSpinBox->setRange(0, 10000);
    samplesSpinBox->setSingleStep(50);
    samplesSpinBox->setValue(0);
    samplesSpinBox->setSpecialValueText("All"); // 0 означает "все время"
    
    // Устанавливаем минимальное значение 100 для предотвращения зависания
    // (0 остается специальным значением "все время")
    samplesSpinBox->setMinimum(0);
    // Добавляем валидатор для предотвращения ввода значений меньше 100 (кроме 0)
    connect(samplesSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onSamplesPerPageValueChanged()));
    samplesControlLayout->addWidget(samplesPerPageLabel);
    samplesControlLayout->addWidget(samplesSpinBox);
    samplesLayout->addLayout(samplesControlLayout);
    
    mainLayout->addWidget(samplesGroup);
    
    // Группа 3: Color Settings
    QGroupBox* colorGroup = new QGroupBox("", this);
    QVBoxLayout* colorLayout = new QVBoxLayout(colorGroup);
    
    // Color scheme
    QHBoxLayout* schemeLayout = new QHBoxLayout();
    QLabel* colorLabel = new QLabel("Color scheme:", this);
    colorCombo = new QComboBox(this);
    colorCombo->addItems({"gray", "seismic", "BWR", "viridis", "red_blue", "phase", 
                          "amplitude", "spectrum", "petrel_classic", "kingdom", "seisworks"});
    colorCombo->setCurrentText("gray");
    schemeLayout->addWidget(colorLabel);
    schemeLayout->addWidget(colorCombo);
    colorLayout->addLayout(schemeLayout);
    
    mainLayout->addWidget(colorGroup);
    
    // Группа 4: Image Settings
    QGroupBox* imageGroup = new QGroupBox("", this);
    QVBoxLayout* imageLayout = new QVBoxLayout(imageGroup);
    
    // Gain
    QHBoxLayout* gainLayout = new QHBoxLayout();
    QLabel* gainLabel = new QLabel("Gain:", this);
    gainSpinBox = new QDoubleSpinBox(this);
    gainSpinBox->setRange(0.1, 50.0);
    gainSpinBox->setSingleStep(0.1);
    gainSpinBox->setDecimals(1);
    gainSpinBox->setValue(1.0);
    gainLayout->addWidget(gainLabel);
    gainLayout->addWidget(gainSpinBox);
    imageLayout->addLayout(gainLayout);
    
    mainLayout->addWidget(imageGroup);
    
    // Группа 5: Grid Settings
    QGroupBox* gridGroup = new QGroupBox("", this);
    QVBoxLayout* gridLayout = new QVBoxLayout(gridGroup);
    
    // Grid checkbox
    QHBoxLayout* gridLayout2 = new QHBoxLayout();
    QLabel* gridLabel = new QLabel("Grid:", this);
    gridCheckBox = new QCheckBox(this);
    gridCheckBox->setChecked(false); // По умолчанию сетка отключена
    gridLayout2->addWidget(gridLabel);
    gridLayout2->addWidget(gridCheckBox);
    gridLayout->addLayout(gridLayout2);
    
    mainLayout->addWidget(gridGroup);
    
    // Группа 6: File Info (без заголовка)
    QGroupBox* fileInfoGroup = new QGroupBox("", this);
    QVBoxLayout* fileInfoLayout = new QVBoxLayout(fileInfoGroup);
    
    // Samples и dt в одной строке
    QHBoxLayout* fileInfoRow = new QHBoxLayout();
    
    // Samples info
    QLabel* samplesInfoLabel = new QLabel("Samples:", this);
    samplesLabel = new QLabel("No file", this);
    samplesLabel->setStyleSheet("QLabel { color: gray; }");
    fileInfoRow->addWidget(samplesInfoLabel);
    fileInfoRow->addWidget(samplesLabel);
    
    // Добавляем небольшой отступ между Samples и dt
    fileInfoRow->addSpacing(15);
    
    // dt info
    QLabel* dtInfoLabel = new QLabel("dt:", this);
    dtLabel = new QLabel("No file", this);
    dtLabel->setStyleSheet("QLabel { color: gray; }");
    fileInfoRow->addWidget(dtInfoLabel);
    fileInfoRow->addWidget(dtLabel);
    
    fileInfoLayout->addLayout(fileInfoRow);
    
    // Добавляем растягивающийся элемент перед File Info для выравнивания по правому краю
    mainLayout->addStretch();
    
    mainLayout->addWidget(fileInfoGroup);
    
    // Создаем таймер для дебаунсинга samples per page
    samplesDebounceTimer = new QTimer(this);
    samplesDebounceTimer->setSingleShot(true);
    samplesDebounceTimer->setInterval(500); // 500ms задержка
    
    // Подключаем сигналы
    connect(tracesSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onTracesPerPageChanged()));
    connect(samplesSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onSamplesPerPageChanged()));
    connect(colorCombo, SIGNAL(currentTextChanged(QString)), this, SLOT(onColorSchemeChanged()));
    connect(gainSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onGainChanged()));
    connect(gridCheckBox, SIGNAL(toggled(bool)), this, SLOT(onGridEnabledChanged()));
    
    // Подключаем таймер дебаунсинга
    connect(samplesDebounceTimer, SIGNAL(timeout()), this, SLOT(onSamplesPerPageDebounced()));
}

// Геттеры
int SettingsPanel::getTracesPerPage() const {
    return tracesSpinBox->value();
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

bool SettingsPanel::getGridEnabled() const {
    return gridCheckBox->isChecked();
}

// Сеттеры
void SettingsPanel::setTracesPerPage(int value) {
    tracesSpinBox->setValue(value);
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

void SettingsPanel::setGridEnabled(bool enabled) {
    gridCheckBox->setChecked(enabled);
}

void SettingsPanel::setFileInfo(int samples, float dt) {
    if (samples > 0) {
        samplesLabel->setText(QString::number(samples));
        samplesLabel->setStyleSheet("QLabel { color: black; }");
        
        // Отображаем dt в микросекундах с единицами измерения
        QString dtText = QString::number(dt, 'f', 1) + " ms";
        dtLabel->setText(dtText);
        dtLabel->setStyleSheet("QLabel { color: black; }");
    } else {
        samplesLabel->setText("No file");
        samplesLabel->setStyleSheet("QLabel { color: gray; }");
        
        dtLabel->setText("No file");
        dtLabel->setStyleSheet("QLabel { color: gray; }");
    }
}

// Слоты для синхронизации
void SettingsPanel::onTracesPerPageChanged() {
    emit settingsChanged();
}

void SettingsPanel::onSamplesPerPageValueChanged() {
    int value = samplesSpinBox->value();
    
    // Если пользователь ввел значение меньше 100 (но не 0), устанавливаем 100
    if (value > 0 && value < 100) {
        samplesSpinBox->setValue(100);
        return; // Не запускаем таймер дебаунсинга, так как значение изменилось программно
    }
    
    // Запускаем таймер дебаунсинга только если значение корректное
    samplesDebounceTimer->start();
}

void SettingsPanel::onSamplesPerPageChanged() {
    // Этот слот теперь не используется напрямую, но оставляем для совместимости
}

void SettingsPanel::onSamplesPerPageDebounced() {
    // Отправляем сигнал только после окончания ввода
    emit settingsChanged();
}

void SettingsPanel::onColorSchemeChanged() {
    emit settingsChanged();
}

void SettingsPanel::onGainChanged() {
    emit settingsChanged();
}

void SettingsPanel::onGridEnabledChanged() {
    emit settingsChanged();
}
