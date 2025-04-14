project(OmegaUARTController LANGUAGES C CXX)

include(FetchContent)
find_package(Threads REQUIRED)


FetchContent_Declare(
    OmegaUtilityDriver
    GIT_REPOSITORY https://github.com/Omegaki113r/OmegaUtilityDriver.git
    GIT_TAG origin/main
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(OmegaUtilityDriver)

add_library(OmegaUARTController STATIC ${PROJ_ROOT_DIR}/src/platform/windows/UARTController.cpp)
target_include_directories(OmegaUARTController PUBLIC ${PROJ_ROOT_DIR}/inc)
target_link_libraries(OmegaUARTController PUBLIC 
    OmegaUtilityDriver Threads::Threads
)
target_compile_definitions(OmegaUARTController PUBLIC 
    CONFIG_OMEGA_LOGGING=1
    WINDOWS_UART
)