
if(ESP_PLATFORM)
    set(PROJ_SOURCES src/platform/esp32/UARTController.cpp)
    idf_component_register(
            INCLUDE_DIRS inc priv/inc
            SRCS ${PROJ_SOURCES})

else()
    message(FATAL_ERROR "Platform not defined")
    cmake_minimum_required(VERSION 3.6)
    project(OmegaUARTController)

endif()