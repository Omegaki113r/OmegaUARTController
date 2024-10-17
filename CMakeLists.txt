cmake_minimum_required(VERSION 3.6)
project(OmegaUARTController)

add_library(OmegaUARTController STATIC src/OmegaUARTCOntroller.c)
target_include_directories(OmegaUARTController PUBLIC inc)

install(
    TARGETS OmegaUARTController
    LIBRARY DESTINATION lib
    )

install(
    FILES inc/OmegaUARTController.h
    TYPE INCLUDE
)