cmake_minimum_required(VERSION 3.26)
project(compilation-test)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. testing)
add_executable(compilation-test main.c)
target_link_libraries(compilation-test OmegaUARTController)