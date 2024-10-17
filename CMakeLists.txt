cmake_minimum_required(VERSION 3.6)
project(OmegaUARTController)

add_library(OmegaUARTController STATIC src/OmegaUARTCOntroller.c)
target_include_directories(OmegaUARTController PUBLIC inc)
# set_target_properties(OmegaUARTController PROPERTIES PUBLIC_HEADER inc/OmegaUARTController.h)

# Library
# Note: may not work on windows
install(
    TARGETS OmegaUARTController
    LIBRARY DESTINATION lib
    )

install(
    FILES inc/OmegaUARTController.h
    TYPE INCLUDE
)