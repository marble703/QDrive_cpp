cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

if(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Debug")
endif()
# Build the driver library
add_library(qdrive_driver
	io/can/can.cpp
	io/serial/serial.cpp
	interface/base/interfacebase.cpp
	interface/interface.cpp
)

add_library(QDrive::driver ALIAS qdrive_driver)

target_include_directories(qdrive_driver
	PUBLIC
			${CMAKE_CURRENT_SOURCE_DIR}/io/can
			${CMAKE_CURRENT_SOURCE_DIR}/io/serial
			${CMAKE_CURRENT_SOURCE_DIR}/interface

)

target_compile_features(qdrive_driver PUBLIC cxx_std_17)

