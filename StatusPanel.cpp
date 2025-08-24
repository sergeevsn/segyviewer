#include "StatusPanel.hpp"
#include <QHBoxLayout>
#include <QLabel>

StatusPanel::StatusPanel(QWidget *parent)
    : QWidget(parent), label(new QLabel("Trace: -, Sample: -, Amp: -", this))
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4,2,4,2);
    layout->addWidget(label);
    setLayout(layout);
}

void StatusPanel::updateInfo(int traceIndex, int sampleIndex, float amplitude) {
    label->setText(QString("Trace: %1 | Sample: %2 | Amp: %3")
                   .arg(traceIndex).arg(sampleIndex).arg(amplitude, 0, 'f', 4));
}

