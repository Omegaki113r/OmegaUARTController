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
  * Last Modified: Sunday, 20th April 2025 8:37:28 am
  * Modified By: 0m3g4ki113r (omegaki113r@gmail.com)
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

int main()
{

	for (;;)
	{
		const auto ports = ::Omega::UART::get_available_ports();
		for (const auto& port : ports)
		{
			OMEGA_LOGD("%s || %s", port.m_friendly_portname, port.m_port_name);
		}

		const auto handle = ::Omega::UART::init("COM8");
		if (0 == handle)
		{
			OMEGA_LOGE("invalid handle");
			return -1;
		}
		::Omega::UART::add_on_connected_callback(handle, []() {OMEGA_LOGI("Connected");});
		::Omega::UART::add_on_disconnected_callback(handle, []() {OMEGA_LOGI("Disonnected");});
		::Omega::UART::connect(handle);
		OMEGA_LOGI("Connection Status: %d", ::Omega::UART::is_connected(handle));
		size_t counter{ 0 };
		const auto read_callback = [&counter](const ::Omega::UART::Handle handle, const u8* data, const size_t data_length)
			{
				OMEGA_HEX_LOGD((void*)data, data_length);
				counter += data_length;
			};
		::Omega::UART::start(handle, read_callback);
		while(counter < 100)
		{ }
		::Omega::UART::stop(handle);
		::Omega::UART::disconnect(handle);
		::Omega::UART::deinit(handle);
	}
	return 0;
}