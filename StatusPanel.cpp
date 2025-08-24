#include "StatusPanel.hpp"
#include <QHBoxLayout>
#include <QLabel>

StatusPanel::StatusPanel(QWidget *parent)
    : QWidget(parent), label(new QLabel("Trace: -, Time: -, Amp: -", this))
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4,2,4,2);
    layout->addWidget(label);
    setLayout(layout);
}

void StatusPanel::updateInfo(int traceIndex, int sampleIndex, float amplitude, float dt) {
    // SampleInterval в SEG-Y измеряется в микросекундах
    // Вычисляем время в миллисекундах: sample * dt (dt уже в микросекундах)
    float timeMs = sampleIndex * dt;
    label->setText(QString("Trace: %1 | Time: %2 ms | Amp: %3")
                   .arg(traceIndex).arg(timeMs, 0, 'f', 2).arg(amplitude, 0, 'f', 4));
}

