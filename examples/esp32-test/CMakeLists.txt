# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set( EXTRA_COMPONENT_DIRS ../..)

add_compile_definitions(CONFIG_OMEGA_LOGGING=1)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32-test C CXX)


# set(UART_CONTROLLER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
# add_subdirectory(${UART_CONTROLLER_DIR} uart_controller)

# add_executable(esp32-test main/main.cpp)

# target_link_libraries(esp32-test OmegaUARTController)
