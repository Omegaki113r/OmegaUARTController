set(PROJ_SOURCES ${PROJ_ROOT_DIR}/src/platform/esp32/UARTController.cpp)
idf_component_register(
    INCLUDE_DIRS        ${PROJ_ROOT_DIR}/inc
    SRCS                ${PROJ_SOURCES}
    REQUIRES            driver OmegaUtilityDriver)
add_compile_definitions(ESP32XX_UART)