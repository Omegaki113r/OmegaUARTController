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
 * Last Modified: Monday, 21st October 2024 3:18:14 am
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

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

    typedef enum
    {
        eDATA_BITS_5,
        eDATA_BITS_6,
        eDATA_BITS_7,
        eDATA_BITS_8,
        eDATA_BITS_MAX,
    } OmegaUARTDataBits;
    typedef enum
    {
        ePARITY_DISABLE,
        ePARITY_EVEN,
        ePARITY_ODD,
        ePARITY_MAX,
    } OmegaUARTParity;
    typedef enum
    {
        eSTOP_BITS_1,
        eSTOP_BITS_1_5,
        eSTOP_BITS_2,
        eSTOP_BITS_MAX,
    } OmegaUARTStopBits;
    typedef uint64_t OmegaUARTHandle;

    typedef struct
    {
        bool m_started;
        uint32_t m_baud_rate;
        OmegaUARTDataBits m_data_bits;
        OmegaUARTParity m_parity;
        OmegaUARTStopBits m_stop_bits;
        void (*read_uart)(uint8_t *, size_t, uint32_t);
    } OmegaUARTController_t;

    OmegaUARTHandle OmegaUARTController_init(OmegaUARTController_t *);
#ifdef __cplusplus
}
#endif

#endif // __OMEGA_UART_CONTROLLER_H__