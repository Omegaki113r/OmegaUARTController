/**
 * @file OmegaUARTController.h
 * @author Omegaki113r
 * @date Thursday, 17th October 2024 3:33:52 pm
 * @copyright Copyright 2024 - 2024 0m3g4ki113r, Xtronic
 * */
/*
 * Project: OmegaUARTController
 * File Name: OmegaUARTController.h
 * File Created: Thursday, 17th October 2024 3:33:52 pm
 * Author: Omegaki113r (omegaki113r@gmail.com)
 * -----
 * Last Modified: Wednesday, 11th December 2024 9:58:29 am
 * Modified By: Omegaki113r (omegaki113r@gmail.com)
 * -----
 * Copyright 2024 - 2024 0m3g4ki113r, Xtronic
 * -----
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	---------------------------------------------------------
 */

#ifndef __OMEGA_UART_CONTROLLER_H__
#define __OMEGA_UART_CONTROLLER_H__

#include <cstdint>

#include "OmegaUtilityDriver.hpp"

namespace Omega
{

    enum class UARTDataBits
    {
        eDATA_BITS_5,
        eDATA_BITS_6,
        eDATA_BITS_7,
        eDATA_BITS_8
    };
    enum class UARTParity
    {
        ePARITY_DISABLE,
        ePARITY_EVEN,
        ePARITY_ODD,
        ePARITY_MAX,
    };
    enum class UARTStopBits
    {
        eSTOP_BITS_1,
        eSTOP_BITS_1_5,
        eSTOP_BITS_2,
        eSTOP_BITS_MAX,
    };

    typedef uint64_t UARTHandle;

    struct UARTController
    {
        bool m_started;
        uint32_t m_baud_rate;
        UARTDataBits m_data_bits;
        UARTParity m_parity;
        UARTStopBits m_stop_bits;
        OmegaGPIO_t m_tx_pin;
        OmegaGPIO_t m_rx_pin;
        size_t (*read_uart)(uint8_t *, size_t, uint32_t);
        size_t (*write_uart)(uint8_t *, size_t, uint32_t);
    };

    [[nodiscard]] UARTHandle UARTController_init(UARTController *);

} // namespace Omega

#endif // __OMEGA_UART_CONTROLLER_H__