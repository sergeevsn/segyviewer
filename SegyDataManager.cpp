#include "SegyDataManager.hpp"
#include "SegyReader.hpp"
#include <iostream>
#include <algorithm>
#include <limits>
#include <cmath>

SegyDataManager::SegyDataManager(int cacheSize)
    : cacheSize(cacheSize), totalTraces(0), 
      globalMinAmplitude(0.0f), globalMaxAmplitude(1.0f), globalStatsValid(false) {
}

bool SegyDataManager::loadFile(const std::string& filename) {
    this->filename = filename;
    try {
        reader = std::unique_ptr<SegyReader>(new SegyReader(filename));
        totalTraces = reader->num_traces();
        
        // Очищаем кэш при загрузке нового файла
        clearCache();
        

        return true;
        
    } catch (const std::exception& e) {
        return false;
    }
}

std::vector<std::vector<float>> SegyDataManager::getTracesPage(int page, int tracesPerPage) const {
    int start = page * tracesPerPage;
    if (start >= totalTraces) return {};
    int end = std::min(start + tracesPerPage, totalTraces);
    
    return getTracesRange(start, end - start);
}

std::vector<std::vector<float>> SegyDataManager::getTracesRange(int startTrace, int count) const {
    if (startTrace < 0 || startTrace >= totalTraces) return {};
    int end = std::min(startTrace + count, totalTraces);
    
    std::vector<std::vector<float>> result;
    result.reserve(end - startTrace);
    
    for (int i = startTrace; i < end; ++i) {
        result.push_back(getTraceFromCache(i));
    }
    
    return result;
}

std::vector<float> SegyDataManager::getTraceFromCache(int traceIndex) const {
    // Проверяем, есть ли трасса в кэше
    auto it = traceCache.find(traceIndex);
    if (it != traceCache.end()) {
        // Обновляем LRU
        updateLRU(traceIndex);
        return it->second;
    }
    
    // Загружаем трассу из файла
    try {
        std::vector<float> trace = reader->get_trace(traceIndex);
        addToCache(traceIndex, trace);
        return trace;
    } catch (const std::exception& e) {
        return {};
    }
}

void SegyDataManager::addToCache(int traceIndex, const std::vector<float>& trace) const {
    // Если кэш полон, удаляем самую старую трассу
    if (traceCache.size() >= static_cast<size_t>(cacheSize)) {
        evictOldest();
    }
    
    // Добавляем новую трассу в кэш
    traceCache[traceIndex] = trace;
    lruList.push_back(traceIndex);
}

void SegyDataManager::evictOldest() const {
    if (lruList.empty()) return;
    
    int oldestTrace = lruList.front();
    lruList.pop_front();
    traceCache.erase(oldestTrace);
}

void SegyDataManager::updateLRU(int traceIndex) const {
    // Удаляем из списка LRU
    lruList.remove(traceIndex);
    // Добавляем в конец (самая недавно использованная)
    lruList.push_back(traceIndex);
}

std::vector<uint8_t> SegyDataManager::getTraceHeader(int traceIndex) const {
    if (traceIndex < 0 || traceIndex >= totalTraces || !reader) {
        return {};
    }
    
    try {
        return reader->get_trace_header(traceIndex);
    } catch (const std::exception& e) {
        return {};
    }
}

void SegyDataManager::setCacheSize(int size) {
    if (size < 1) size = 1;
    cacheSize = size;
    
    // Если новый размер меньше текущего, удаляем лишние трассы
    while (traceCache.size() > static_cast<size_t>(cacheSize)) {
        evictOldest();
    }
}

void SegyDataManager::clearCache() {
    traceCache.clear();
    lruList.clear();
}

void SegyDataManager::computeGlobalStats(int numTraces) {
    if (!reader || totalTraces == 0) return;
    
    // Ограничиваем количество трасс для анализа
    int tracesToAnalyze = std::min(numTraces, totalTraces);
    
    globalMinAmplitude = std::numeric_limits<float>::max();
    globalMaxAmplitude = std::numeric_limits<float>::lowest();
    
    for (int i = 0; i < tracesToAnalyze; ++i) {
        try {
            std::vector<float> trace = reader->get_trace(i);
            for (float amplitude : trace) {
                if (std::isfinite(amplitude)) {
                    globalMinAmplitude = std::min(globalMinAmplitude, amplitude);
                    globalMaxAmplitude = std::max(globalMaxAmplitude, amplitude);
                }
            }
        } catch (const std::exception& e) {
            // Игнорируем ошибки при вычислении статистики
        }
    }
    
    // Если все значения одинаковые, устанавливаем небольшой диапазон
    if (std::abs(globalMaxAmplitude - globalMinAmplitude) < 1e-6) {
        globalMaxAmplitude = globalMinAmplitude + 1.0f;
    }
    
    globalStatsValid = true;
}


