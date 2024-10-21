cmake_minimum_required(VERSION 3.6)
project(OmegaUARTController)

file(
        DOWNLOAD https://raw.githubusercontent.com/Omegaki113r/OmegaUtilityDriver/refs/heads/main/inc/OmegaUtilityDriver.h 
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/OmegaUtilityDriver.h
    )

add_library(OmegaUARTController STATIC src/OmegaUARTCOntroller.c)
target_include_directories(OmegaUARTController PUBLIC inc ${CMAKE_CURRENT_SOURCE_DIR}/inc/OmegaUtilityDriver.h)

install(
    TARGETS OmegaUARTController
    LIBRARY DESTINATION lib
    )

install(
    FILES inc/OmegaUARTController.h
    TYPE INCLUDE
)