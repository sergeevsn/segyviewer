cmake_minimum_required(VERSION 3.5)
project(SegyViewer)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Добавляем флаги компилятора для лучшей совместимости со старыми версиями GCC
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
    endif()
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

# Добавляем путь к заголовочным файлам sgylib
include_directories(sgylib)

add_executable(SegyViewer
    main.cpp
    MainWindow.cpp
    SegyViewer.cpp
    SegyDataManager.cpp
    StatusPanel.cpp
    SettingsDialog.cpp
    SettingsPanel.cpp
    TraceInfoPanel.cpp
    sgylib/SegyReader.cpp
    ColorSchemes.cpp
)

target_link_libraries(SegyViewer
    Qt5::Widgets
)

