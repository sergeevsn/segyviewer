#pragma once
#include <cstdint>
#include <cmath>
#include <cstring>

struct FieldInfo {
    int offset; // 1-based offset
    int size;   // bytes: 2 or 4
};

inline int16_t get_i16_be(const uint8_t* buf, int offset1based) {
    int offset = offset1based - 1;
    return (static_cast<int16_t>(buf[offset]) << 8) | buf[offset + 1];
}

inline int32_t get_i32_be(const uint8_t* buf, int offset1based) {
    int offset = offset1based - 1;
    return (static_cast<int32_t>(buf[offset]) << 24) |
           (static_cast<int32_t>(buf[offset + 1]) << 16) |
           (static_cast<int32_t>(buf[offset + 2]) << 8) |
           buf[offset + 3];
}

#define IEEEMAX 0x7FFFFFFF
#define IEMAXIB 0x611FFFFF
#define IEMINIB 0x21200000

inline float ibm_to_float(uint32_t ibm) {
    static const int it[8] = { 0x21800000, 0x21400000, 0x21000000, 0x21000000,
                               0x20c00000, 0x20c00000, 0x20c00000, 0x20c00000 };
    static const int mt[8] = { 8, 4, 2, 2, 1, 1, 1, 1 };
    unsigned int manthi, iexp, inabs;
    int ix;
    uint32_t u = ibm;

    manthi = u & 0x00ffffff;
    ix     = manthi >> 21;
    iexp   = ( ( u & 0x7f000000 ) - it[ix] ) << 1;
    manthi = manthi * mt[ix] + iexp;
    inabs  = u & 0x7fffffff;
    if ( inabs > IEMAXIB ) manthi = IEEEMAX;
    manthi = manthi | ( u & 0x80000000 );
    u = ( inabs < IEMINIB ) ? 0 : manthi;
    
    float result;
    std::memcpy(&result, &u, sizeof(u));
    return result;
}

inline uint32_t ieee_to_ibm(float val) {
    if (val == 0.0f) return 0;
    uint32_t sign = (val < 0) ? 0x80000000 : 0;
    if (val < 0) val = -val;
    int exponent = 64;
    while (val < 1.0f) { val *= 16.0f; --exponent; }
    while (val >= 1.0f) { val /= 16.0f; ++exponent; }
    uint32_t fraction = static_cast<uint32_t>(val * 0x01000000) & 0x00FFFFFF;
    return sign | (exponent << 24) | fraction;
} 

inline void set_i16_be(uint8_t* buf, int offset1based, int16_t value) {
    int offset = offset1based - 1;
    buf[offset] = static_cast<uint8_t>((value >> 8) & 0xFF);
    buf[offset + 1] = static_cast<uint8_t>(value & 0xFF);
}

inline void set_i32_be(uint8_t* buf, int offset1based, int32_t value) {
    int offset = offset1based - 1;
    buf[offset]     = static_cast<uint8_t>((value >> 24) & 0xFF);
    buf[offset + 1] = static_cast<uint8_t>((value >> 16) & 0xFF);
    buf[offset + 2] = static_cast<uint8_t>((value >> 8) & 0xFF);
    buf[offset + 3] = static_cast<uint8_t>(value & 0xFF);
} 

inline uint32_t get_u32_be(const uint8_t* buf) {
    return (static_cast<uint32_t>(buf[0]) << 24) |
           (static_cast<uint32_t>(buf[1]) << 16) |
           (static_cast<uint32_t>(buf[2]) << 8)  |
           (static_cast<uint32_t>(buf[3]));
} 

inline void put_u32_be(uint8_t* buf, uint32_t value) {
    buf[0] = static_cast<uint8_t>((value >> 24) & 0xFF);
    buf[1] = static_cast<uint8_t>((value >> 16) & 0xFF);
    buf[2] = static_cast<uint8_t>((value >> 8)  & 0xFF);
    buf[3] = static_cast<uint8_t>(value & 0xFF);
}

// Функция для отображения прогресса (отладочный вывод убран)
void print_progress_bar(const std::string& message, int current, int total);