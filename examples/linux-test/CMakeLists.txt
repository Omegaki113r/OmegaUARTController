cmake_minimum_required(VERSION 3.26)
project(linux-test)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. testing)
add_executable(linux-test main.cpp)
target_link_libraries(linux-test OmegaUARTController)