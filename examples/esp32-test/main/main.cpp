#include <cstdio>
#include <cstring>

#include <driver/gpio.h>
#include <driver/uart.h>
#include <esp_err.h>
#include <esp_log.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "OmegaUARTController/UARTController.hpp"

constexpr uart_port_t uart_port_num = UART_NUM_1;
constexpr gpio_num_t uart_rx = GPIO_NUM_17, uart_tx = GPIO_NUM_18;
constexpr int uart_buffer_size = 1024 * 2;
static QueueHandle_t uart_queue_handle;

extern "C" void app_main(void)
{
    Omega::UART::Handle handle = Omega::UART::init(UART_NUM_1, {uart_tx}, {uart_rx});
    if (0 == handle)
    {
        return;
    }

    char expected[] = "Hello world from slave";
    char hello_world[100]{};
    char writing[] = " Hello world from master";
    for (;;)
    {
        size_t read_amount = 100;
        Omega::UART::read(handle, reinterpret_cast<u8 *>(hello_world), &read_amount, 1000);
        OMEGA_LOGD("Read: %s", hello_world);
        if (!std::memcmp(expected, hello_world, sizeof(expected)))
        {
            size_t write_size = std::strlen(writing);
            Omega::UART::write(handle, reinterpret_cast<const u8 *>(writing), &write_size, 100);
        }
        std::memset(hello_world, 0, sizeof(hello_world));
    }
}
