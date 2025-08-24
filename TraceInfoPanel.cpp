#include "TraceInfoPanel.hpp"
#include "sgylib/TraceFieldMap.hpp"
#include <QGroupBox>
#include <QScrollArea>

TraceInfoPanel::TraceInfoPanel(QWidget* parent)
    : QWidget(parent)
{
    // Создаем заголовок
    titleLabel = new QLabel("Trace Header Information", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 12px; padding: 5px;");
    
    // Создаем таблицу
    infoTable = new QTableWidget(this);
    infoTable->setColumnCount(2);
    infoTable->setHorizontalHeaderLabels({"Field", "Value"});
    infoTable->horizontalHeader()->setStretchLastSection(true);
    infoTable->verticalHeader()->setVisible(false);
    infoTable->setAlternatingRowColors(true);
    infoTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    infoTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    // Устанавливаем размеры колонок
    infoTable->setColumnWidth(0, 150);
    
    // Загружаем все поля из TraceFieldMap в правильном порядке
    // Порядок полей как в TraceFieldMap.hpp
    displayFields = {
        "TRACE_SEQUENCE_LINE",
        "TRACE_SEQUENCE_FILE", 
        "FieldRecord",
        "TraceNumber",
        "EnergySourcePoint",
        "CDP",
        "CDP_TRACE",
        "TraceIdentificationCode",
        "NSummedTraces",
        "NStackedTraces",
        "DataUse",
        "offset",
        "ReceiverGroupElevation",
        "SourceSurfaceElevation",
        "SourceDepth",
        "ReceiverDatumElevation",
        "SourceDatumElevation",
        "SourceWaterDepth",
        "GroupWaterDepth",
        "ElevationScalar",
        "SourceGroupScalar",
        "SourceX",
        "SourceY",
        "GroupX",
        "GroupY",
        "CoordinateUnits",
        "WeatheringVelocity",
        "SubWeatheringVelocity",
        "SourceUpholeTime",
        "GroupUpholeTime",
        "SourceStaticCorrection",
        "GroupStaticCorrection",
        "TotalStaticApplied",
        "LagTimeA",
        "LagTimeB",
        "DelayRecordingTime",
        "MuteTimeStart",
        "MuteTimeEND",
        "TRACE_SAMPLE_COUNT",
        "TRACE_SAMPLE_INTERVAL",
        "GainType",
        "InstrumentGainConstant",
        "InstrumentInitialGain",
        "Correlated",
        "SweepFrequencyStart",
        "SweepFrequencyEnd",
        "SweepLength",
        "SweepType",
        "SweepTraceTaperLengthStart",
        "SweepTraceTaperLengthEnd",
        "TaperType",
        "AliasFilterFrequency",
        "AliasFilterSlope",
        "NotchFilterFrequency",
        "NotchFilterSlope",
        "LowCutFrequency",
        "HighCutFrequency",
        "LowCutSlope",
        "HighCutSlope",
        "YearDataRecorded",
        "DayOfYear",
        "HourOfDay",
        "MinuteOfHour",
        "SecondOfMinute",
        "TimeBaseCode",
        "TraceWeightingFactor",
        "GeophoneGroupNumberRoll1",
        "GeophoneGroupNumberFirstTraceOrigField",
        "GeophoneGroupNumberLastTraceOrigField",
        "GapSize",
        "OverTravel",
        "CDP_X",
        "CDP_Y",
        "INLINE_3D",
        "CROSSLINE_3D",
        "ShotPoint",
        "ShotPointScalar",
        "TraceValueMeasurementUnit",
        "TransductionConstantMantissa",
        "TransductionConstantPower",
        "TransductionUnit",
        "TraceIdentifier",
        "ScalarTraceHeader",
        "SourceType",
        "SourceEnergyDirectionVert",
        "SourceEnergyDirectionXline",
        "SourceEnergyDirectionIline",
        "SourceMeasurementMantissa",
        "SourceMeasurementExponent",
        "SourceMeasurementUnit",
        "UnassignedInt1",
        "UnassignedInt2"
    };
    
    setupTable();
    
    // Создаем layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(infoTable);
    layout->setContentsMargins(5, 5, 5, 5);
    
    // Устанавливаем минимальную ширину
    setMinimumWidth(300);
}

void TraceInfoPanel::setupTable() {
    infoTable->setRowCount(displayFields.size());
    
    for (size_t i = 0; i < displayFields.size(); ++i) {
        QTableWidgetItem* fieldItem = new QTableWidgetItem(QString::fromStdString(displayFields[i]));
        QTableWidgetItem* valueItem = new QTableWidgetItem("N/A");
        
        infoTable->setItem(i, 0, fieldItem);
        infoTable->setItem(i, 1, valueItem);
    }
}

void TraceInfoPanel::updateTraceInfo(int traceIndex, const std::vector<uint8_t>& traceHeader) {
    if (traceHeader.empty()) {
        titleLabel->setText("Trace Header Information - No Data");
        return;
    }
    
    titleLabel->setText(QString("Trace Header Information - Trace %1").arg(traceIndex));
    
    // Обновляем значения в таблице
    for (size_t i = 0; i < displayFields.size(); ++i) {
        const std::string& fieldName = displayFields[i];
        QTableWidgetItem* valueItem = infoTable->item(i, 1);
        
        try {
            int32_t value = get_trace_field_value(traceHeader.data(), fieldName);
            valueItem->setText(QString::number(value));
        } catch (const std::exception& e) {
            valueItem->setText("Error");
        }
    }
}

