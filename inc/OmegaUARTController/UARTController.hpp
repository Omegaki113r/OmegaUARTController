/**
 * @file UARTController.h
 * @author Omegaki113r
 * @date Thursday, 17th October 2024 3:33:52 pm
 * @copyright Copyright 2024 - 2024 0m3g4ki113r, Xtronic
 * */
/*
 * Project: OmegaUARTController
 * File Name: UARTController.h
 * File Created: Thursday, 17th October 2024 3:33:52 pm
 * Author: Omegaki113r (omegaki113r@gmail.com)
 * -----
 * Last Modified: Friday, 10th January 2025 3:09:09 pm
 * Modified By: Omegaki113r (omegaki113r@gmail.com)
 * -----
 * Copyright 2024 - 2024 0m3g4ki113r, Xtronic
 * -----
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	---------------------------------------------------------
 */

#pragma once

#include <cstdint>

#include <driver/uart.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "OmegaUtilityDriver.hpp"

namespace Omega
{
    namespace UART
    {
        enum class DataBits
        {
            eDATA_BITS_5 = UART_DATA_5_BITS,
            eDATA_BITS_6 = UART_DATA_6_BITS,
            eDATA_BITS_7 = UART_DATA_7_BITS,
            eDATA_BITS_8 = UART_DATA_8_BITS,
        };
        enum class Parity
        {
            ePARITY_DISABLE = UART_PARITY_DISABLE,
            ePARITY_EVEN = UART_PARITY_EVEN,
            ePARITY_ODD = UART_PARITY_ODD,
        };
        enum class StopBits
        {
            eSTOP_BITS_1 = UART_STOP_BITS_1,
            eSTOP_BITS_1_5 = UART_STOP_BITS_1_5,
            eSTOP_BITS_2 = UART_STOP_BITS_2,
        };

        typedef u64 Handle;
        typedef u32 Baudrate;

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
            size_t (*read_uart)(uint8_t *, size_t, uint32_t);
            size_t (*write_uart)(uint8_t *, size_t, uint32_t);
        };

        [[nodiscard]] Handle init(uart_port_t in_port, OmegaGPIO in_tx, OmegaGPIO in_rx, Baudrate in_baudrate = 115200, DataBits in_databits = DataBits::eDATA_BITS_8, Parity in_parity = Parity::ePARITY_DISABLE, StopBits in_stopbits = StopBits::eSTOP_BITS_1);
    } // namespace UART
} // namespace Omega