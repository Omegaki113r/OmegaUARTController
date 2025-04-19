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
 * Last Modified: Saturday, 19th April 2025 4:16:11 pm
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
#include <functional>
#include <vector>

#include "OmegaUtilityDriver/UtilityDriver.hpp"

#if defined(WINDOWS_UART)

#include <windows.h>

#define UART_DATA_5_BITS 5
#define UART_DATA_6_BITS 6
#define UART_DATA_7_BITS 7
#define UART_DATA_8_BITS 8

#define UART_PARITY_DISABLE NOPARITY
#define UART_PARITY_EVEN EVENPARITY
#define UART_PARITY_ODD ODDPARITY

#define UART_STOP_BITS_1 ONESTOPBIT
#define UART_STOP_BITS_1_5 ONE5STOPBITS
#define UART_STOP_BITS_2 TWOSTOPBITS

#elif defined(MACOSX_UART)

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOBSD.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <mach/mach.h>

#define UART_DATA_5_BITS 5
#define UART_DATA_6_BITS 6
#define UART_DATA_7_BITS 7
#define UART_DATA_8_BITS 8

#define UART_PARITY_DISABLE 0
#define UART_PARITY_EVEN 1
#define UART_PARITY_ODD 2

#define UART_STOP_BITS_1 0
#define UART_STOP_BITS_1_5 1
#define UART_STOP_BITS_2 2

#endif

namespace Omega
{
        namespace UART
        {
                constexpr size_t FRIENDLY_PORT_NAME_SIZE{256};
#if defined(WINDOWS_UART)
                constexpr size_t PORT_NAME_SIZE{32};
#elif defined(MACOSX_UART)
                constexpr size_t PORT_NAME_SIZE{256};
#endif
                struct EnumeratedUARTPort
                {
                        char m_friendly_portname[FRIENDLY_PORT_NAME_SIZE + 1]{0};
#if !defined(ESP32XX_UART)
                        char m_port_name[PORT_NAME_SIZE + 1]{0};
#endif
                };

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
                        ePARITY_ODD = UART_PARITY_ODD,
                        ePARITY_EVEN = UART_PARITY_EVEN,
                };
                enum class StopBits
                {
                        eSTOP_BITS_1 = UART_STOP_BITS_1,
#if !defined(MACOSX_UART)
                        eSTOP_BITS_1_5 = UART_STOP_BITS_1_5,
#endif
                        eSTOP_BITS_2 = UART_STOP_BITS_2,
                };

                typedef u64 Handle;
                typedef u32 Baudrate;

                struct Response
                {
                        OmegaStatus status;
                        size_t size;
                };

#if defined(ESP32XX_UART)
                [[nodiscard]] Handle init(uart_port_t in_port, OmegaGPIO in_tx, OmegaGPIO in_rx, Baudrate in_baudrate = 115200, DataBits in_databits = DataBits::eDATA_BITS_8, Parity in_parity = Parity::ePARITY_DISABLE, StopBits in_stopbits = StopBits::eSTOP_BITS_1);
#elif defined(WINDOWS_UART)
                [[nodiscard]] std::vector<EnumeratedUARTPort> get_available_ports();
                [[nodiscard]] Handle init(const char *in_port, Baudrate in_baudrate = 115200, DataBits in_databits = DataBits::eDATA_BITS_8, Parity in_parity = Parity::ePARITY_DISABLE, StopBits in_stopbits = StopBits::eSTOP_BITS_1);
#elif defined(MACOSX_UART)
                [[nodiscard]] std::vector<EnumeratedUARTPort> get_available_ports();
                [[nodiscard]] Handle init(const char *in_port, Baudrate in_baudrate = 115200, DataBits in_databits = DataBits::eDATA_BITS_8, Parity in_parity = Parity::ePARITY_DISABLE, StopBits in_stopbits = StopBits::eSTOP_BITS_1);
#endif
                OmegaStatus start(Handle in_handle);
                [[nodiscard]] Response read(Handle in_handle, u8 *out_buffer, const size_t in_read_bytes, u32 in_timeout_ms);
                [[nodiscard]] Response write(Handle in_handle, const u8 *in_buffer, const size_t in_write_bytes, u32 in_timeout_ms);
                OmegaStatus deinit(const Handle);
#if defined(ESP32XX_UART)
                __attribute__((weak)) void on_data(const Omega::UART::Handle, const u8 *, const size_t);
#elif defined(WINDOWS_UART)
                OmegaStatus add_on_read_callback(Handle in_handle, std::function<void(const Handle, const u8 *, const size_t)> in_callback);
#elif defined(MACOSX_UART)
                OmegaStatus add_on_read_callback(Handle in_handle, std::function<void(const Handle, const u8 *, const size_t)> in_callback);
#endif
        } // namespace UART
} // namespace Omega
