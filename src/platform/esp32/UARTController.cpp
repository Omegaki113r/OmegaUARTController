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
 * Last Modified: Friday, 10th January 2025 8:40:50 pm
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

namespace Omega
{
    namespace UART
    {
        internal std::unordered_map<Handle, UARTController> s_controllers;

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
            if (ESP_OK != uart_param_config(in_port, &uart_config))
            {
                LOGE("uart_param_config failed");
                return 0;
            }
            if (ESP_OK != uart_set_pin(in_port, tx, rx, GPIO_NUM_NC, GPIO_NUM_NC))
            {
                LOGE("uart_set_pin failed");
                return 0;
            }
            // if (ESP_OK != uart_driver_install(in_port, s_controllers[handle].rx_buffer_size, s_controllers[handle].tx_buffer_size, 10, &s_controllers[handle].queue_handle, 0))
            // {
            //     LOGE("uart_driver_install failed");
            //     return 0;
            // }
            return handle;
        }

        OmegaStatus read(Handle in_handle, u8 *out_buffer, size_t *in_out_read_bytes, const u32 in_timeout_ms)
        {
            if (0 == in_handle)
            {
                LOGE("Provided handle is invalid: %lld", in_handle);
                return eFAILED;
            }
            if (nullptr == out_buffer || nullptr == in_out_read_bytes || 0 == *in_out_read_bytes)
            {
                LOGE("provided buffer is invalid");
                return eFAILED;
            }
            auto iterator = s_controllers.find(in_handle);
            if (iterator == s_controllers.end())
            {
                LOGE("Provided handle cannot be found");
                return eFAILED;
            }
            auto &controller = iterator->second;
            if (!controller.started)
            {
                if (ESP_OK != uart_driver_install(controller.m_uart_port, controller.rx_buffer_size, controller.tx_buffer_size, controller.queue_event_count, &controller.queue_handle, 0))
                {
                    LOGE("uart_driver_install failed");
                    return eFAILED;
                }
                controller.started = true;
            }
            *in_out_read_bytes = uart_read_bytes(controller.m_uart_port, out_buffer, *in_out_read_bytes, pdMS_TO_TICKS(in_timeout_ms));
            return eSUCCESS;
        }

        OmegaStatus write(Handle in_handle, const u8 *in_buffer, size_t *in_out_write_bytes, u32 in_timeout_ms)
        {
            if (0 == in_handle)
            {
                LOGE("Provided handle is invalid: %lld", in_handle);
                return eFAILED;
            }
            if (nullptr == in_buffer || nullptr == in_out_write_bytes || 0 == *in_out_write_bytes)
            {
                LOGE("provided buffer is invalid");
                return eFAILED;
            }
            auto iterator = s_controllers.find(in_handle);
            if (iterator == s_controllers.end())
            {
                LOGE("Provided handle cannot be found");
                return eFAILED;
            }
            auto &controller = iterator->second;
            if (!controller.started)
            {
                if (ESP_OK != uart_driver_install(controller.m_uart_port, controller.rx_buffer_size, controller.tx_buffer_size, controller.queue_event_count, &controller.queue_handle, 0))
                {
                    LOGE("uart_driver_install failed");
                    return eFAILED;
                }
                controller.started = true;
            }
            *in_out_write_bytes = uart_write_bytes(controller.m_uart_port, in_buffer, *in_out_write_bytes);
            if (ESP_OK != uart_wait_tx_done(controller.m_uart_port, in_timeout_ms))
            {
                LOGD("uart_wait_tx_done failed");
                return eFAILED;
            }
            return eSUCCESS;
        }
    } // namespace UART
} // namespace Omega