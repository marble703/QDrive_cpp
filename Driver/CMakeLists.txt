cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

if(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(qdrive_driver
	io/can/can.cpp
	io/serial/serial.cpp
	interface/base/interfacebase.cpp
	interface/interface.cpp
	motor/motor.cpp
	logger/logger.cpp
)

add_library(QDrive::driver ALIAS qdrive_driver)

target_include_directories(qdrive_driver
	PUBLIC
			${CMAKE_CURRENT_SOURCE_DIR}/io/can
			${CMAKE_CURRENT_SOURCE_DIR}/io/serial
			${CMAKE_CURRENT_SOURCE_DIR}/interface
			${CMAKE_CURRENT_SOURCE_DIR}/motor
			${CMAKE_CURRENT_SOURCE_DIR}/logger
			${PROJECT_SOURCE_DIR}/third_party/spdlog/include
)

if(EXISTS "${PROJECT_SOURCE_DIR}/third_party/spdlog/CMakeLists.txt")
    if(NOT TARGET spdlog::spdlog_header_only)
        add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/spdlog ${CMAKE_CURRENT_BINARY_DIR}/spdlog)
    endif()
else()
    message("Please try to init submodule")
endif()

target_link_libraries(qdrive_driver PUBLIC spdlog::spdlog_header_only)

target_compile_features(qdrive_driver PUBLIC cxx_std_17)
