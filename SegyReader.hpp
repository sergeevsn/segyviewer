#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
#include <stdexcept>

class SegyReader {
public:
    /**
     * @brief Основной конструктор. Открывает SEG-Y файл для чтения.
     * @param filename Путь к SEG-Y файлу.
     */
    explicit SegyReader(const std::string& filename);
    ~SegyReader();

    // Запрещаем копирование и присваивание, т.к. класс управляет файловым ресурсом.
    SegyReader(const SegyReader&) = delete;
    SegyReader& operator=(const SegyReader&) = delete;

    // --- ОСНОВНЫЕ МЕТОДЫ ДОСТУПА К ДАННЫМ ---
    std::vector<float> get_trace(int index) const;
    std::vector<uint8_t> get_trace_header(int index) const;

    // --- ГЕТТЕРЫ И ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ ---
    int num_traces() const { return num_traces_; }
    int num_samples() const { return num_samples_; }
    float sample_interval() const { return sample_interval_; }

    int32_t get_header_value_i32(int trace_index, const std::string& key) const;
    int32_t get_header_value_i32(const std::vector<uint8_t>& trace_header, const std::string& key) const;
    int16_t get_header_value_i16(const std::vector<uint8_t>& trace_header, const std::string& key) const;

    int32_t get_bin_header_value_i32(const std::string& key) const;
    int16_t get_bin_header_value_i16(const std::string& key) const;

    const std::vector<char>& text_header() const { return text_header_; }
    const std::vector<uint8_t>& bin_header() const { return bin_header_; }

private:
    // Константы SEG-Y формата
    static const int TEXT_HEADER_SIZE = 3200;
    static const int BINARY_HEADER_SIZE = 400;
    static const int TRACE_HEADER_SIZE = 240;

    // Вспомогательные методы для чтения данных
    uint32_t read_u32_be(const uint8_t* data) const;
    uint16_t read_u16_be(const uint8_t* data) const;
    int32_t read_i32_be(const uint8_t* data) const;
    int16_t read_i16_be(const uint8_t* data) const;
    float ibm_to_float(uint32_t ibm) const;
    
    // Вычисление смещений в файле
    std::streamoff data_offset() const { return TEXT_HEADER_SIZE + BINARY_HEADER_SIZE; }
    std::streamoff trace_offset(int index) const { return data_offset() + static_cast<std::streamoff>(index) * trace_bsize_; }
    std::streamoff trace_data_offset(int index) const { return trace_offset(index) + TRACE_HEADER_SIZE; }

    std::string filename_;
    mutable std::fstream file_;
    std::vector<char> text_header_;
    std::vector<uint8_t> bin_header_;
    int num_traces_ = 0;
    int num_samples_ = 0;
    float sample_interval_ = 0.0f;
    int trace_bsize_ = 0;
};
