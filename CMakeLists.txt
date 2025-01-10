
if(ESP_PLATFORM)
    set(PROJ_SOURCES src/platform/esp32/UARTController.cpp)
    idf_component_register(
            INCLUDE_DIRS inc
            SRCS ${PROJ_SOURCES}
            REQUIRES driver OmegaUtilityDriver)

else()
    message(FATAL_ERROR "Platform not defined")
    cmake_minimum_required(VERSION 3.6)
    project(OmegaUARTController)

endif()