#pragma once

#include <stdexcept>

template<typename T>
class Optional {
private:
    bool has_value_;
    T value_;

public:
    // Конструктор по умолчанию - создает пустой optional
    Optional() : has_value_(false), value_() {}
    
    // Конструктор с значением
    Optional(const T& value) : has_value_(true), value_(value) {}
    
    // Конструктор перемещения
    Optional(T&& value) : has_value_(true), value_(std::move(value)) {}
    
    // Проверка наличия значения
    bool has_value() const { return has_value_; }
    
    // Оператор bool для проверки наличия значения
    explicit operator bool() const { return has_value_; }
    
    // Получение значения
    const T& value() const {
        if (!has_value_) {
            throw std::runtime_error("Optional has no value");
        }
        return value_;
    }
    
    T& value() {
        if (!has_value_) {
            throw std::runtime_error("Optional has no value");
        }
        return value_;
    }
    
    // Оператор разыменования
    const T& operator*() const { return value(); }
    T& operator*() { return value(); }
    
    // Оператор доступа к членам
    const T* operator->() const { return &value(); }
    T* operator->() { return &value(); }
    
    // Получение значения или значение по умолчанию
    T value_or(const T& default_value) const {
        return has_value_ ? value_ : default_value;
    }
};

// Создание пустого optional
template<typename T>
Optional<T> nullopt() {
    return Optional<T>();
}

// Глобальная константа для пустого optional (только для int)
const Optional<int> nullopt_int = Optional<int>();
