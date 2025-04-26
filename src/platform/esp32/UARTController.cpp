/**
 * @file UARTController.cpp
 * @author Omegaki113r
 * @date Thursday, 17th October 2024 3:34:02 pm
 * @copyright Copyright 2024 - 2024 0m3g4ki113r, Xtronic
 * */
/*
 * Project: OmegaUARTController
 * File Name: UARTController.cpp
 * File Created: Thursday, 17th October 2024 3:34:02 pm
 * Author: Omegaki113r (omegaki113r@gmail.com)
 * -----
 * Last Modified: Saturday, 26th April 2025 3:07:01 pm
 * Modified By: Omegaki113r (omegaki113r@gmail.com)
 * -----
 * Copyright 2024 - 2024 0m3g4ki113r, Xtronic
 * -----
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	---------------------------------------------------------
 */

#include <unordered_map>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <driver/gpio.h>
#include <driver/uart.h>
#include <esp_err.h>

#include <sdkconfig.h>

#include "OmegaUARTController/UARTController.hpp"

#if CONFIG_OMEGA_UART_CONTROLLER_DEBUG
#define LOGD(format, ...) OMEGA_LOGD(format, ##__VA_ARGS__)
#else
#define LOGD(format, ...)
#endif

#if CONFIG_OMEGA_UART_CONTROLLER_INFO
#define LOGI(format, ...) OMEGA_LOGI(format, ##__VA_ARGS__)
#else
#define LOGI(format, ...)
#endif

#if CONFIG_OMEGA_UART_CONTROLLER_WARN
#define LOGW(format, ...) OMEGA_LOGW(format, ##__VA_ARGS__)
#else
#define LOGW(format, ...)
#endif

#if CONFIG_OMEGA_UART_CONTROLLER_ERROR
#define LOGE(format, ...) OMEGA_LOGE(format, ##__VA_ARGS__)
#else
#define LOGE(format, ...)
#endif

__internal__ constexpr size_t s_UART_BASE_STACK_SIZE = 256;
#define UART_EVENT_HANDLER_STACK_SIZE (configMINIMAL_STACK_SIZE + s_UART_BASE_STACK_SIZE * 4)

namespace Omega
{
    namespace UART
    {
        __attribute__((constructor)) void on_uart_driver_created()
        {
        }

        struct UARTController
        {
            uart_port_t m_uart_port;
            OmegaGPIO m_tx_pin;
            OmegaGPIO m_rx_pin;
            Baudrate m_baudrate;
            DataBits m_databits;
            Parity m_parity;
            StopBits m_stopbits;
            Handle handle;
            OmegaGPIO rts_pin;
            OmegaGPIO cts_pin;
            size_t tx_buffer_size = 1024 * 2;
            size_t rx_buffer_size = 1024 * 2;
            bool started;
            QueueHandle_t queue_handle;
            size_t queue_event_count = 10;
            TaskHandle_t m_uart_event_task_handle;
            u8 *rx_buffer;
            size_t (*read_uart)(uint8_t *, size_t, uint32_t);
            size_t (*write_uart)(uint8_t *, size_t, uint32_t);
        };

        __internal__ std::unordered_map<Handle, UARTController> s_controllers;

        __internal__ inline OmegaStatus initialize_stack(UARTController &controller, const uart_config_t &in_config)
        {
            if (ESP_OK != uart_param_config(controller.m_uart_port, &in_config))
            {
                LOGE("uart_param_config failed");
                return eFAILED;
            }
            if (ESP_OK != uart_set_pin(controller.m_uart_port, controller.m_tx_pin.pin, controller.m_rx_pin.pin, GPIO_NUM_NC, GPIO_NUM_NC))
            {
                LOGE("uart_set_pin failed");
                return eFAILED;
            }
            controller.rx_buffer = static_cast<u8 *>(omega_malloc(sizeof(u8) * controller.rx_buffer_size));
            if (nullptr == controller.rx_buffer)
            {
                return eFAILED;
            }
            if (ESP_OK != uart_driver_install(controller.m_uart_port, controller.rx_buffer_size, controller.tx_buffer_size, controller.queue_event_count, &controller.queue_handle, 0))
            {
                LOGE("uart_driver_install failed");
                return eFAILED;
            }

            auto uart_event_handler = [](void *arg)
            {
                auto controller = static_cast<UARTController *>(arg);
                for (;;)
                {
                    uart_event_t uart_event{};
                    if (nullptr == controller->queue_handle)
                    {
                        delay({0, 0, 0, 500});
                        continue;
                    }
                    xQueueReceive(controller->queue_handle, &uart_event, portMAX_DELAY);
                    switch (uart_event.type)
                    {
                    case UART_DATA: /*!< UART data event*/
                    {
                        LOGD("UART_DATA");
                        const size_t read_bytes = uart_read_bytes(controller->m_uart_port, controller->rx_buffer, uart_event.size, portMAX_DELAY);
                        on_data(controller->handle, controller->rx_buffer, read_bytes);
                        break;
                    }
                    case UART_BREAK: /*!< UART break event*/
                    {
                        LOGD("UART_BREAK");
                        break;
                    }
                    case UART_BUFFER_FULL: /*!< UART RX buffer full event*/
                    {
                        LOGD("UART_BUFFER_FULL");
                        break;
                    }
                    case UART_FIFO_OVF: /*!< UART FIFO overflow event*/
                    {
                        LOGD("UART_FIFO_OVF");
                        break;
                    }
                    case UART_FRAME_ERR: /*!< UART RX frame error event*/
                    {
                        LOGD("UART_FRAME_ERR");
                        break;
                    }
                    case UART_PARITY_ERR: /*!< UART RX parity event*/
                    {
                        LOGD("UART_PARITY_ERR");
                        break;
                    }
                    case UART_DATA_BREAK: /*!< UART TX data and break event*/
                    {
                        LOGD("UART_DATA_BREAK");
                        break;
                    }
                    case UART_PATTERN_DET: /*!< UART pattern detected */
                    {
                        LOGD("UART_PATTERN_DET");
                        break;
                    }
                    default:
                    {
                        LOGE("Unhandled UART event");
                        break;
                    }
                    }
                }
                vTaskDelete(NULL);
            };
            if (pdPASS != xTaskCreate(uart_event_handler, "user_event_handler", UART_EVENT_HANDLER_STACK_SIZE, &controller, configMAX_PRIORITIES - 5, &controller.m_uart_event_task_handle))
            {
                LOGE("xTaskCreate(uart_event_handler) failed");
                return eFAILED;
            }
            controller.started = true;
            return eSUCCESS;
        }

        Handle init(uart_port_t in_port, OmegaGPIO in_tx, OmegaGPIO in_rx, Baudrate in_baudrate, DataBits in_databits, Parity in_parity, StopBits in_stopbits)
        {
            if (UART_NUM_MAX <= in_port)
            {
                LOGE("Invalid UART port");
                return 0;
            }
            if (0 >= in_baudrate)
            {
                LOGE("Invalid baudrate: %ld", in_baudrate);
                return 0;
            }
            const auto tx = static_cast<gpio_num_t>(in_tx.pin);
            const auto rx = static_cast<gpio_num_t>(in_rx.pin);
            if (GPIO_NUM_MAX <= tx || GPIO_NUM_MAX <= rx)
            {
                LOGE("Invalid GPIOs provided. TX: %d, RX: %d", tx, rx);
                return 0;
            }
            for (const auto &[handle, controller] : s_controllers)
            {
                if (in_port == controller.m_uart_port || in_tx == controller.m_tx_pin || in_rx == controller.m_rx_pin)
                {
                    LOGE("UART controller is already initialized with given parameters");
                    LOGE("Found=> Handle: %lld PortNumber:%d TX:%ld RX:%ld", controller.handle, static_cast<int>(controller.m_uart_port), static_cast<u32>(controller.m_tx_pin.pin), static_cast<u32>(controller.m_rx_pin.pin));
                    LOGE("Provided=> PortNumber:%d TX:%d RX:%d", in_port, tx, rx);
                    return 0;
                }
            }

            const auto handle = OmegaUtilityDriver_generate_handle();
            if (0 == handle)
            {
                LOGE("Handle creation failed");
                return 0;
            }
            s_controllers[handle] = {in_port, in_tx, in_rx, in_baudrate, in_databits, in_parity, in_stopbits, handle};
            const uart_config_t uart_config = {static_cast<int>(in_baudrate), static_cast<uart_word_length_t>(in_databits), static_cast<uart_parity_t>(in_parity), static_cast<uart_stop_bits_t>(in_stopbits)};
            if (eSUCCESS != initialize_stack(s_controllers[handle], uart_config))
            {
                LOGE("initialize_stack failed");
                return 0;
            }
            return handle;
        }

        bool connected(Handle in_handle)
        {
            if (const auto found = s_controllers.find(in_handle); s_controllers.end() != found)
            {
                return true;
            }
            return false;
        }

        Response read(Handle in_handle, u8 *out_buffer, const size_t in_read_bytes, const u32 in_timeout_ms)
        {
            if (0 == in_handle)
            {
                LOGE("Provided handle is invalid: %lld", in_handle);
                return {eFAILED};
            }
            if (nullptr == out_buffer || 0 == in_read_bytes)
            {
                LOGE("provided buffer is invalid");
                return {eFAILED};
            }
            auto iterator = s_controllers.find(in_handle);
            if (iterator == s_controllers.end())
            {
                LOGE("Provided handle cannot be found");
                return {eFAILED};
            }
            auto &controller = iterator->second;
            const size_t read_bytes = uart_read_bytes(controller.m_uart_port, out_buffer, in_read_bytes, pdMS_TO_TICKS(in_timeout_ms));
            return {eSUCCESS, read_bytes};
        }

        Response write(Handle in_handle, const u8 *in_buffer, const size_t in_write_bytes, u32 in_timeout_ms)
        {
            if (0 == in_handle)
            {
                LOGE("Provided handle is invalid: %lld", in_handle);
                return {eFAILED};
            }
            if (nullptr == in_buffer || 0 == in_write_bytes)
            {
                LOGE("provided buffer is invalid");
                return {eFAILED};
            }
            auto iterator = s_controllers.find(in_handle);
            if (iterator == s_controllers.end())
            {
                LOGE("Provided handle cannot be found");
                return {eFAILED};
            }
            auto &controller = iterator->second;
            const size_t write_bytes = uart_write_bytes(controller.m_uart_port, in_buffer, in_write_bytes);
            if (ESP_OK != uart_wait_tx_done(controller.m_uart_port, in_timeout_ms))
            {
                LOGD("uart_wait_tx_done failed");
                return {eFAILED};
            }
            return {eSUCCESS, write_bytes};
        }

        [[nodiscard]] OmegaStatus deinit(const Handle in_handle)
        {
            if (0 == in_handle)
            {
                LOGE("Provided handle is invalid: %lld", in_handle);
                return {eFAILED};
            }
            auto iterator = s_controllers.find(in_handle);
            if (iterator == s_controllers.end())
            {
                LOGE("Provided handle cannot be found");
                return {eFAILED};
            }
            auto &controller = iterator->second;

            vTaskDelete(controller.m_uart_event_task_handle);
            if (ESP_OK != uart_driver_delete(controller.m_uart_port))
            {
                LOGE("uart_driver_delete failed");
                return eFAILED;
            }
            return eSUCCESS;
        }

        __attribute__((weak)) void on_data(const Omega::UART::Handle, const u8 *, const size_t) {}
    } // namespace UART
} // namespace Omega
