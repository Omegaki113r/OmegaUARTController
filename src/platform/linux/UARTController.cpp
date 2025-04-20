/**
 * @file UARTController.cpp
 * @author 0m3g4ki113r
 * @date Sunday, 20th April 2025 8:32:30 am
 * @copyright Copyright 2024 - 2025 0m3g4ki113r, Xtronic
 * */
/*
 * Project: OmegaUARTController
 * File Name: UARTController.cpp
 * File Created: Sunday, 20th April 2025 8:32:30 am
 * Author: 0m3g4ki113r (omegaki113r@gmail.com)
 * -----
 * Last Modified: Sunday, 20th April 2025 8:42:33 am
 * Modified By: 0m3g4ki113r (omegaki113r@gmail.com)
 * -----
 * Copyright 2024 - 2025 0m3g4ki113r, Xtronic
 * -----
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	---------------------------------------------------------
 */

#include "OmegaUtilityDriver/UtilityDriver.hpp"
#include "OmegaUARTController/UARTController.hpp"

namespace Omega
{
    namespace UART
    {
        std::vector<EnumeratedUARTPort> get_available_ports()
        {
            return {};
        }

        Handle init(const char *in_port, Baudrate in_baudrate, DataBits in_databits, Parity in_parity, StopBits in_stopbits)
        {
            return 0;
        }

        Response read(Handle in_handle, u8 *out_buffer, const size_t in_read_bytes, u32 in_timeout_ms)
        {
            return {eFAILED, 0};
        }

        Response write(Handle in_handle, const u8 *in_buffer, const size_t in_write_bytes, u32 in_timeout_ms)
        {
            return {eFAILED, 0};
        }

        OmegaStatus add_on_read_callback(Handle in_handle, std::function<void(const Handle, const u8 *, const size_t)> in_callback)
        {
            return eFAILED;
        }
        OmegaStatus start(Handle in_handle)
        {
            return eFAILED;
        }
        OmegaStatus deinit(const Handle in_handle)
        {
            return eFAILED;
        }
    } // namespace UART
} // namespace Omega
