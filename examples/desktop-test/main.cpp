/**
 * @file main.c
 * @author Omegaki113r
 * @date Thursday, 17th October 2024 7:03:04 pm
 * @copyright Copyright 2024 - 2024 0m3g4ki113r, Xtronic
 * */
/*
 * Project: OmegaUARTController
 * File Name: main.c
 * File Created: Thursday, 17th October 2024 7:03:04 pm
 * Author: Omegaki113r (omegaki113r@gmail.com)
 * -----
 * Last Modified: Thursday, 17th October 2024 7:19:46 pm
 * Modified By: Omegaki113r (omegaki113r@gmail.com)
 * -----
 * Copyright 2024 - 2024 0m3g4ki113r, Xtronic
 * -----
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	---------------------------------------------------------
 */
#include <stdio.h>

#include "OmegaUARTController/UARTController.hpp"
#include "OmegaUtilityDriver/UtilityDriver.hpp"

void main() 
{

	const auto ports = ::Omega::UART::omega_get_available_ports();
	for (const auto& port : ports)
	{
		OMEGA_LOGD("%s || %s", port.m_friendly_portname, port.m_port_name);
	}

	const auto handle = ::Omega::UART::init("COM8");

	const auto read_callback = [](const ::Omega::UART::Handle handle, const u8* data, const size_t data_length) {
		OMEGA_HEX_LOGD((void*)data, data_length);
	};
	::Omega::UART::add_on_read_callback(handle, read_callback);
	::Omega::UART::start(handle);
	for (;;)
	{
		//char buffer[100+1]{ 0 };
		//::Omega::UART::read(handle, (u8*)buffer, 1, 0);
		//printf("%s",buffer);
	}
	::Omega::UART::deinit(handle);
}