cmake_minimum_required(VERSION 3.22)
project(qdrive_tool_serial)

add_executable(qdrive_serial_tool
    serial_tool.cpp
)

target_compile_features(qdrive_serial_tool PRIVATE cxx_std_17)

target_link_libraries(qdrive_serial_tool PRIVATE qdrive_driver)

target_include_directories(qdrive_serial_tool PRIVATE
    ${CMAKE_SOURCE_DIR}/Driver/io
)
