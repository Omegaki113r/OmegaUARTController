cmake_minimum_required(VERSION 3.25)



set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(PROJ_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})


if(ESP_PLATFORM)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/ESP32xx.cmake)
else()
    if(WIN32)
        include(${CMAKE_CURRENT_LIST_DIR}/cmake/Windows.cmake)
    elseif(APPLE)
        include(${CMAKE_CURRENT_LIST_DIR}/cmake/Apple.cmake)
    elseif(LINUX)
        message("Error")
    else()
        message("Error")
    endif()
endif()