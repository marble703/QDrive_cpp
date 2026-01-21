cmake_minimum_required(VERSION 3.22)
project(qdrive_tool_serial)

function(add_qdrive_tool_executable target_name source_file)
    add_executable(${target_name} ${source_file})
    target_compile_features(${target_name} PRIVATE cxx_std_17)
    target_link_libraries(${target_name} PRIVATE qdrive_driver)
endfunction()

add_qdrive_tool_executable(qdrive_tool_serial io/serial_tool.cpp)
add_qdrive_tool_executable(qdrive_tool_can io/can_tool.cpp)