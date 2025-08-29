#include "SegyReader.hpp"
#include "SegyUtil.hpp"
#include <algorithm>
#include <cstring>

SegyReader::SegyReader(const std::string& filename) : filename_(filename) {
    // Проверяем, что файл существует
    std::ifstream test_file(filename);
    if (!test_file.good()) {
        throw std::runtime_error("File does not exist or is not accessible: " + filename);
    }
    test_file.close();
    
    // Открываем файл для чтения
    file_.open(filename, std::ios::binary | std::ios::in);
    if (!file_.is_open()) {
        throw std::runtime_error("Cannot open SEG-Y file: " + filename);
    }
    
    // Проверяем размер файла
    file_.seekg(0, std::ios::end);
    std::streamoff file_size = file_.tellg();
    file_.seekg(0, std::ios::beg);
    
    if (file_size < TEXT_HEADER_SIZE + BINARY_HEADER_SIZE) {
        throw std::runtime_error("File too small to be a valid SEG-Y file");
    }

    // Читаем текстовый заголовок (3200 байт)
    text_header_.resize(TEXT_HEADER_SIZE);
    file_.read(text_header_.data(), TEXT_HEADER_SIZE);

    // Читаем бинарный заголовок (400 байт)
    bin_header_.resize(BINARY_HEADER_SIZE);
    file_.read(reinterpret_cast<char*>(bin_header_.data()), BINARY_HEADER_SIZE);

    // Получаем информацию о трассах из бинарного заголовка
    try {
        num_samples_ = get_bin_header_value_i16("SamplesPerTrace");
        sample_interval_ = get_bin_header_value_i16("SampleInterval") / 1000.0f; // в микросекундах
        
        if (num_samples_ <= 0) {
            throw std::runtime_error("Invalid number of samples per trace: " + std::to_string(num_samples_));
        }
    } catch (const std::exception& e) {
        throw;
    }

    // Вычисляем размер одной трассы и общее количество трасс
    trace_bsize_ = TRACE_HEADER_SIZE + num_samples_ * 4; // 4 байта на сэмпл (IBM float)
    
    num_traces_ = (file_size - data_offset()) / trace_bsize_;

    if (num_traces_ <= 0) {
        throw std::runtime_error("Invalid number of traces: " + std::to_string(num_traces_));
    }
}

SegyReader::~SegyReader() {
    if (file_.is_open()) {
        file_.close();
    }
}

std::vector<float> SegyReader::get_trace(int index) const {
    if (index < 0 || index >= num_traces_) {
        throw std::out_of_range("Trace index out of range: " + std::to_string(index));
    }

    std::vector<float> trace_data(num_samples_);
    std::streamoff offset = trace_data_offset(index);
    
    // Читаем данные трассы
    std::vector<uint8_t> buf(num_samples_ * 4);
    file_.seekg(offset, std::ios::beg);
    file_.read(reinterpret_cast<char*>(buf.data()), buf.size());

    // Конвертируем IBM float в IEEE float
    for (int i = 0; i < num_samples_; ++i) {
        uint32_t ibm = read_u32_be(&buf[i * 4]);
        trace_data[i] = ibm_to_float(ibm);
    }
    
    return trace_data;
}

std::vector<uint8_t> SegyReader::get_trace_header(int index) const {
    if (index < 0 || index >= num_traces_) {
        throw std::out_of_range("Trace index out of range: " + std::to_string(index));
    }

    std::vector<uint8_t> header(TRACE_HEADER_SIZE);
    std::streamoff offset = trace_offset(index);
    
    file_.seekg(offset, std::ios::beg);
    file_.read(reinterpret_cast<char*>(header.data()), TRACE_HEADER_SIZE);
    
    return header;
}

int32_t SegyReader::get_header_value_i32(int trace_index, const std::string& key) const {
    auto header = get_trace_header(trace_index);
    return get_header_value_i32(header, key);
}

int32_t SegyReader::get_header_value_i32(const std::vector<uint8_t>& trace_header, const std::string& key) const {
    // Простая реализация для основных полей заголовка трассы
    if (key == "TRACE_SEQUENCE_LINE") return read_i32_be(&trace_header[0]);
    if (key == "TRACE_SEQUENCE_FILE") return read_i32_be(&trace_header[4]);
    if (key == "FieldRecord") return read_i32_be(&trace_header[8]);
    if (key == "TraceNumber") return read_i32_be(&trace_header[12]);
    if (key == "EnergySourcePoint") return read_i32_be(&trace_header[16]);
    if (key == "CDP") return read_i32_be(&trace_header[20]);
    if (key == "CDP_TRACE") return read_i32_be(&trace_header[24]);
    if (key == "offset") return read_i32_be(&trace_header[36]);
    
    throw std::invalid_argument("Unknown header field: " + key);
}

int16_t SegyReader::get_header_value_i16(const std::vector<uint8_t>& trace_header, const std::string& key) const {
    // Простая реализация для 16-битных полей
    if (key == "TraceIdentificationCode") return read_i16_be(&trace_header[28]);
    if (key == "NSummedTraces") return read_i16_be(&trace_header[30]);
    if (key == "NStackedTraces") return read_i16_be(&trace_header[32]);
    if (key == "DataUse") return read_i16_be(&trace_header[34]);
    if (key == "TRACE_SAMPLE_COUNT") return read_i16_be(&trace_header[114]);
    if (key == "TRACE_SAMPLE_INTERVAL") return read_i16_be(&trace_header[116]);
    
    throw std::invalid_argument("Unknown header field: " + key);
}

int32_t SegyReader::get_bin_header_value_i32(const std::string& key) const {
    // Простая реализация для полей бинарного заголовка
    if (key == "JobID") return read_i32_be(&bin_header_[0]);
    if (key == "LineNumber") return read_i32_be(&bin_header_[4]);
    if (key == "ReelNumber") return read_i32_be(&bin_header_[8]);
    
    throw std::invalid_argument("Unknown binary header field: " + key);
}

int16_t SegyReader::get_bin_header_value_i16(const std::string& key) const {
    // Простая реализация для 16-битных полей бинарного заголовка
    if (key == "DataTracesPerEnsemble") return read_i16_be(&bin_header_[12]);
    if (key == "AuxTracesPerEnsemble") return read_i16_be(&bin_header_[14]);
    if (key == "SampleInterval") return read_i16_be(&bin_header_[16]);
    if (key == "SampleIntervalOriginal") return read_i16_be(&bin_header_[18]);
    if (key == "SamplesPerTrace") return read_i16_be(&bin_header_[20]);
    if (key == "SamplesPerTraceOriginal") return read_i16_be(&bin_header_[22]);
    if (key == "DataSampleFormat") return read_i16_be(&bin_header_[24]);
    if (key == "EnsembleFold") return read_i16_be(&bin_header_[26]);
    if (key == "SortingCode") return read_i16_be(&bin_header_[28]);
    if (key == "VerticalSumCode") return read_i16_be(&bin_header_[30]);
    
    throw std::invalid_argument("Unknown binary header field: " + key);
}

// Вспомогательные методы для чтения данных
uint32_t SegyReader::read_u32_be(const uint8_t* data) const {
    return (static_cast<uint32_t>(data[0]) << 24) |
           (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8) |
           static_cast<uint32_t>(data[3]);
}

uint16_t SegyReader::read_u16_be(const uint8_t* data) const {
    return (static_cast<uint16_t>(data[0]) << 8) |
           static_cast<uint16_t>(data[1]);
}

int32_t SegyReader::read_i32_be(const uint8_t* data) const {
    return static_cast<int32_t>(read_u32_be(data));
}

int16_t SegyReader::read_i16_be(const uint8_t* data) const {
    return static_cast<int16_t>(read_u16_be(data));
}


