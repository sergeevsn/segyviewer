#include "StatusPanel.hpp"
#include <QHBoxLayout>
#include <QLabel>

StatusPanel::StatusPanel(QWidget *parent)
    : QWidget(parent), 
      traceLabel(new QLabel("Trace: -, Time: -, Amp: -", this)),
      zoomLabel(new QLabel("Zoom: Left drag to select, Right click to reset, Double click to reset", this))
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4,2,4,2);
    
    // Лейбл с информацией о трассе слева
    layout->addWidget(traceLabel);
    
    // Растягивающийся элемент между лейблами
    layout->addStretch();
    
    // Лейбл с информацией о зуме справа
    layout->addWidget(zoomLabel);
    
    setLayout(layout);
}

void StatusPanel::updateInfo(int traceIndex, int sampleIndex, float amplitude, float dt) {
    // SampleInterval в SEG-Y измеряется в микросекундах
    // Вычисляем время в миллисекундах: sample * dt (dt уже в микросекундах)
    float timeMs = sampleIndex * dt;
    traceLabel->setText(QString("Trace: %1 | Time: %2 ms | Amp: %3")
                   .arg(traceIndex).arg(timeMs, 0, 'f', 2).arg(amplitude, 0, 'f', 4));
}

void StatusPanel::showZoomHelp() {
    zoomLabel->setText("Zoom Help: Left drag to select area, Right click to reset, Double click to reset, Menu: View → Reset Zoom");
}

