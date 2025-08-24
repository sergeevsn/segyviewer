#include "SettingsDialog.hpp"
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QFormLayout>

SettingsDialog::SettingsDialog(const ViewerSettings& initial, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Viewer Settings");

    tracesSpin = new QSpinBox(this);
    tracesSpin->setRange(1, 5000);
    tracesSpin->setValue(initial.tracesPerPage);

    stepSpin = new QSpinBox(this);
    stepSpin->setRange(1, 1000);
    stepSpin->setValue(initial.navigationStep);

    colorCombo = new QComboBox(this);
    colorCombo->addItems({"Grayscale", "Red", "White"});
    int idx = colorCombo->findText(initial.colorScheme);
    if (idx >= 0) colorCombo->setCurrentIndex(idx);

    gainSpin = new QDoubleSpinBox(this);
    gainSpin->setRange(0.1, 50.0); // Увеличили максимальное значение до 50
    gainSpin->setSingleStep(0.1);
    gainSpin->setDecimals(1);
    gainSpin->setValue(initial.gain);

    QFormLayout* form = new QFormLayout;
    form->addRow("Traces per page:", tracesSpin);
    form->addRow("Navigation step:", stepSpin);
    form->addRow("Color scheme:", colorCombo);
    form->addRow("Gain:", gainSpin);

    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
    setLayout(layout);
}

ViewerSettings SettingsDialog::getSettings() const {
    ViewerSettings s;
    s.tracesPerPage = tracesSpin->value();
    s.navigationStep = stepSpin->value();
    s.colorScheme = colorCombo->currentText();
    s.gain = gainSpin->value();
    return s;
}

