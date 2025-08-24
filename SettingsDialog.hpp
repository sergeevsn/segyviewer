#pragma once
#include <QDialog>
#include <QDoubleSpinBox>

class QSpinBox;
class QComboBox;

struct ViewerSettings {
    int tracesPerPage;
    int navigationStep;
    QString colorScheme; // "Grayscale", "Red", "White"
    float gain; // Множитель для усиления контрастности
};

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(const ViewerSettings& initial, QWidget* parent = nullptr);
    ViewerSettings getSettings() const;

private:
    QSpinBox* tracesSpin;
    QSpinBox* stepSpin;
    QComboBox* colorCombo;
    QDoubleSpinBox* gainSpin;
};

