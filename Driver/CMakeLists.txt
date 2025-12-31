cmake_minimum_required(VERSION 3.22)

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
			${CMAKE_CURRENT_SOURCE_DIR}/io
			${CMAKE_CURRENT_SOURCE_DIR}/interface

)

target_compile_features(qdrive_driver PUBLIC cxx_std_17)

