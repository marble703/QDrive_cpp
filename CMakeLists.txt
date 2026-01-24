cmake_minimum_required(VERSION 3.22)
project(QDrive_cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release")
endif()

option(WITHOUT_GUI "Disable building GUI" OFF)

if(WITHOUT_GUI STREQUAL "ON")
    message(STATUS "GUI build is disabled (WITHOUT_GUI=ON)")
    set(GUI_DISABLED ON)
else()
    set(GUI_DISABLED OFF)
endif()

option(WITHOUT_LOGGER "Disable building logger" OFF)

if(WITHOUT_LOGGER STREQUAL "ON")
    message(STATUS "Logger build is disabled (WITHOUT_LOGGER=ON)")
    set(LOGGER_DISABLED ON)
else()
    set(LOGGER_DISABLED OFF)
endif()

if(NOT LOGGER_DISABLED )
    if(EXISTS "${PROJECT_SOURCE_DIR}/third_party/spdlog/CMakeLists.txt")
        add_subdirectory(third_party/spdlog)
        set(SPDLOG_HEADER_ONLY ON)
    else()
        find_package(spdlog REQUIRED)
    endif()
    add_subdirectory(example)
endif()

if(LOGGER_DISABLED)
    add_compile_definitions(WITHOUT_LOGGER)
endif()

add_subdirectory(Driver)
add_subdirectory(tool)

