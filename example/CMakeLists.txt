cmake_minimum_required(VERSION 3.22)

add_executable(qdrive_example
	src/main.cpp
)

target_compile_features(qdrive_example PRIVATE cxx_std_17)
target_link_libraries(qdrive_example PRIVATE qdrive_driver)

add_executable(qdrive_example_reader
	src/reader.cpp)

target_compile_features(qdrive_example_reader PRIVATE cxx_std_17)
target_link_libraries(qdrive_example_reader PRIVATE qdrive_driver)