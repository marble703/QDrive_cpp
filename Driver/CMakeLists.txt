cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

if(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(DRIVER_SOURCES
	io/can/can.cpp
	io/serial/serial.cpp
	interface/base/interfacebase.cpp
	interface/interface.cpp
	motor/motor.cpp
)

# 添加 logger 源文件
if(NOT LOGGER_DISABLED)
	list(APPEND DRIVER_SOURCES logger/logger.cpp)
endif()

add_library(qdrive_driver ${DRIVER_SOURCES})

target_include_directories(qdrive_driver
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/io/can
		${CMAKE_CURRENT_SOURCE_DIR}/io/serial
		${CMAKE_CURRENT_SOURCE_DIR}/interface
		${CMAKE_CURRENT_SOURCE_DIR}/motor
		${CMAKE_CURRENT_SOURCE_DIR}/logger
)

# 添加spdlog头文件路径
if(NOT LOGGER_DISABLED)
	target_include_directories(qdrive_driver
		PUBLIC
			${PROJECT_SOURCE_DIR}/third_party/spdlog/include
	)
endif()

target_compile_features(qdrive_driver PUBLIC cxx_std_17)
