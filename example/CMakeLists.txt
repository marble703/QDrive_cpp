cmake_minimum_required(VERSION 3.22)
project(qdrive_example)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

if(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

function(add_qdrive_executable target_name source_file)
    add_executable(${target_name} ${source_file})
    target_compile_features(${target_name} PRIVATE cxx_std_17)
    target_link_libraries(${target_name} PRIVATE qdrive_driver)
endfunction()

add_qdrive_executable(qdrive_example src/main.cpp)
add_qdrive_executable(qdrive_example_reader src/reader.cpp)
add_qdrive_executable(qdrive_example_two src/two.cpp)
add_qdrive_executable(qdrive_example_can src/main_can.cpp)
add_qdrive_executable(qdrive_example_can_reader src/reader_can.cpp)