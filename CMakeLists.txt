cmake_minimum_required(VERSION 3.6)
project(OmegaUARTController)

add_library(OmegaUARTController STATIC src/OmegaUARTCOntroller.c)
target_include_directories(OmegaUARTController PUBLIC inc)