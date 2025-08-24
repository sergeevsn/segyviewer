#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <list>
#include "SegyReader.hpp"

class SegyDataManager {
public:
    SegyDataManager(int cacheSize = 1000);
    ~SegyDataManager() = default;
    
    bool loadFile(const std::string& filename);
    std::vector<std::vector<float>> getTracesPage(int page, int tracesPerPage) const;
    std::vector<std::vector<float>> getTracesRange(int startTrace, int count) const;
    std::vector<uint8_t> getTraceHeader(int traceIndex) const;
    int traceCount() const { return totalTraces; }
    
    // Настройки кэша
    void setCacheSize(int size);
    int getCacheSize() const { return cacheSize; }
    void clearCache();
    
    // Глобальные статистики амплитуд (на основе первых N трасс)
    void computeGlobalStats(int numTraces = 1000);
    float getGlobalMinAmplitude() const { return globalMinAmplitude; }
    float getGlobalMaxAmplitude() const { return globalMaxAmplitude; }
    bool hasGlobalStats() const { return globalStatsValid; }

private:
    // LRU кэш для трасс
    mutable std::unordered_map<int, std::vector<float>> traceCache;
    mutable std::list<int> lruList; // Список для LRU логики
    int cacheSize;
    
    // Данные файла
    std::string filename;
    std::unique_ptr<SegyReader> reader;
    int totalTraces;
    
    // Глобальные статистики
    float globalMinAmplitude;
    float globalMaxAmplitude;
    bool globalStatsValid;
    
    // Методы кэширования
    std::vector<float> getTraceFromCache(int traceIndex) const;
    void addToCache(int traceIndex, const std::vector<float>& trace) const;
    void evictOldest() const;
    void updateLRU(int traceIndex) const;
};

