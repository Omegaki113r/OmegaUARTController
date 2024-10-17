cmake_minimum_required(VERSION 3.6)
project(OmegaUARTController)

add_library(OmegaUARTController STATIC src/OmegaUARTCOntroller.c)
target_include_directories(OmegaUARTController PUBLIC inc)

# Library
# Note: may not work on windows
install(
    TARGETS OmegaUARTController
    LIBRARY DESTINATION lib
    )

install(
    FILES inc/OmegaUARTController.h
    TYPE INCLUDE
    COMPONENT tinyxml2_development
)