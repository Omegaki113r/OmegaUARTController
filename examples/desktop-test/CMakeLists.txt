cmake_minimum_required(VERSION 3.26)
project(desktop-test)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. testing)
add_executable(desktop-test main.cpp)
target_link_libraries(desktop-test OmegaUARTController)