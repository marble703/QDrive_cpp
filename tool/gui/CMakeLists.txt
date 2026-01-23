cmake_minimum_required(VERSION 3.22)
project(qdrive_gui)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/imgui)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)

add_library(imgui_combo 
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_combo PUBLIC 
    ${IMGUI_DIR} 
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui_combo PUBLIC glfw OpenGL::GL)

add_executable(qdrive_gui main.cpp)
target_include_directories(qdrive_gui PRIVATE 
    ${imgui_SOURCE_DIR} 
    ${imgui_SOURCE_DIR}/backends
    ../../Driver
)
target_link_libraries(qdrive_gui PRIVATE imgui_combo qdrive_driver)
