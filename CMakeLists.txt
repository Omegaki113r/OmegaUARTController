cmake_minimum_required(VERSION 3.6)
project(OmegaUARTController)

add_library(OmegaUARTController STATIC src/OmegaUARTCOntroller.c)
target_include_directories(OmegaUARTController PUBLIC inc)

# Binaries
install(
        TARGETS cmake_examples_deb_bin
        DESTINATION bin
        )

# Library
# Note: may not work on windows
install(
    TARGETS cmake_examples_deb
    LIBRARY DESTINATION lib
    )