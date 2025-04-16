cmake_minimum_required(VERSION 3.26)
project(macosx-test)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. testing)
add_executable(macosx-test main.cpp)
target_link_libraries(macosx-test OmegaUARTController)

set_target_properties(macosx-test PROPERTIES MACOSX_BUNDLE TRUE)