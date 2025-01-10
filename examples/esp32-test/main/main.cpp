#include <cstdio>
#include <cstring>

#include <driver/gpio.h>
#include <driver/uart.h>
#include <esp_err.h>
#include <esp_log.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "OmegaUARTController/UARTController.hpp"

const Omega::UART::Handle handle = Omega::UART::init(UART_NUM_1, {GPIO_NUM_18}, {GPIO_NUM_17});

extern "C" void app_main(void)
{
    for (;;)
    {
        delay(1_s);
    }
}

void Omega::UART::on_data(const Omega::UART::Handle handle, const u8 *read_buffer, const size_t read_length)
{
    OMEGA_LOGD("Handle: %lld", handle);
    OMEGA_HEX_LOGD((u8 *)read_buffer, read_length);

    char expected[] = "Hello world from slave";
    char writing[] = " Hello world from master";
    if (!std::memcmp(expected, read_buffer, sizeof(expected)))
    {
        UNUSED(Omega::UART::write(handle, reinterpret_cast<const u8 *>(writing), std::strlen(writing), 100));
    }
}