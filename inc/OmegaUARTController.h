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
 * Last Modified: Thursday, 17th October 2024 7:48:45 pm
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

#include <stddef.h>
#include <stdint.h>

    typedef enum
    {
        e8BITS = 0,
    } OmegaUARTDataBits;
    typedef uint64_t OmegaUARTHandle;

    typedef struct
    {
        void (*read_uart)(uint8_t *, size_t, uint32_t);
    } OmegaUARTController_t;

    OmegaUARTHandle OmegaUARTController_init(OmegaUARTController_t *);
#ifdef __cplusplus
}
#endif

#endif // __OMEGA_UART_CONTROLLER_H__