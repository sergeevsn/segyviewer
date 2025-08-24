#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHeaderView>
#include <vector>
#include <string>

class TraceInfoPanel : public QWidget {
    Q_OBJECT

public:
    explicit TraceInfoPanel(QWidget* parent = nullptr);
    
    // Обновляет информацию о трассе
    void updateTraceInfo(int traceIndex, const std::vector<uint8_t>& traceHeader);

private:
    QTableWidget* infoTable;
    QLabel* titleLabel;
    
    // Список полей для отображения (наиболее важные)
    std::vector<std::string> displayFields;
    
    void setupTable();
    void addFieldRow(const std::string& fieldName, const std::string& value);
};


