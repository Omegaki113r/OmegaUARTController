/**
 * @file UARTController.cpp
 * @author Omegaki113r
 * @date Monday, 14th April 2025 6:40:03 am
 * @copyright Copyright 2024 - 2025 0m3g4ki113r, Xtronic
 * */
/*
 * Project: OmegaUARTController
 * File Name: UARTController.cpp
 * File Created: Monday, 14th April 2025 6:40:03 am
 * Author: Omegaki113r (omegaki113r@gmail.com)
 * -----
 * Last Modified: Monday, 14th April 2025 12:05:15 pm
 * Modified By: Omegaki113r (omegaki113r@gmail.com)
 * -----
 * Copyright 2024 - 2025 0m3g4ki113r, Xtronic
 * -----
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	---------------------------------------------------------
 */

#include <cstring>
#include <functional>
#include <thread>
#include <unordered_map>

#include <windows.h>

#include "OmegaUARTController/UARTController.hpp"
#include "OmegaUtilityDriver/UtilityDriver.hpp"

namespace Omega
{
	namespace UART
	{
		struct UARTPort
		{
			HANDLE m_handle;
			char port_name[100]{0};
			Baudrate m_baudrate{115200};
			DataBits m_databits{DataBits::eDATA_BITS_8};
			StopBits m_stopbits{StopBits::eSTOP_BITS_1};
			Parity m_parity{Parity::ePARITY_DISABLE};
			std::vector<std::function<void(const Handle, const u8 *, const size_t)>> m_read_callbacks;
			std::thread *m_uart_read_thread{nullptr};
		};

		__internal__ std::unordered_map<Handle, UARTPort> s_com_ports;

		[[nodiscard]] Handle init(const char *in_port, Baudrate in_baudrate, DataBits in_databits, Parity in_parity, StopBits in_stopbits)
		{
			__internal__ Handle user_serial_handle = 0;
			HANDLE serial_handle = 0;
			if (serial_handle = CreateFile(in_port, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr); INVALID_HANDLE_VALUE == serial_handle)
			{
				OMEGA_LOGE("Opening Serialport failed. Reason: %s", ERROR_FILE_NOT_FOUND == GetLastError() ? "COMPORT NOT FOUND" : "OPENING COMPORT FAILED");
				return user_serial_handle;
			}
			user_serial_handle++;
			UARTPort serial_port{
				.m_handle = serial_handle,
				.m_baudrate = in_baudrate,
				.m_databits = in_databits,
				.m_stopbits = in_stopbits,
				.m_parity = in_parity,
			};
			UNUSED(std::strncpy(serial_port.port_name, in_port, sizeof(serial_port.port_name)));
			UNUSED(s_com_ports.insert({user_serial_handle, serial_port}));

			DCB dcb_parameters{};
			if (!GetCommState(serial_handle, &dcb_parameters))
			{
				OMEGA_LOGE("Retrieving COMPORT state failed");
				deinit(user_serial_handle);
				return user_serial_handle;
			}
			dcb_parameters.BaudRate = in_baudrate;
			dcb_parameters.ByteSize = static_cast<BYTE>(in_databits);
			dcb_parameters.StopBits = static_cast<BYTE>(in_stopbits);
			dcb_parameters.Parity = static_cast<BYTE>(in_parity);
			if (!SetCommState(serial_handle, &dcb_parameters))
			{
				OMEGA_LOGE("Setting COMPORT state failed");
				deinit(user_serial_handle);
				return user_serial_handle;
			}
			COMMTIMEOUTS timeout_parameters{};
			timeout_parameters.ReadIntervalTimeout = 50;
			timeout_parameters.ReadTotalTimeoutConstant = 50;
			timeout_parameters.ReadTotalTimeoutMultiplier = 10;
			timeout_parameters.WriteTotalTimeoutConstant = 50;
			timeout_parameters.WriteTotalTimeoutMultiplier = 10;
			if (!SetCommTimeouts(serial_handle, &timeout_parameters))
			{
				OMEGA_LOGE("Setting timeouts failed");
				deinit(user_serial_handle);
				return eFAILED;
			}
			return user_serial_handle;
		}

		OmegaStatus start(Handle in_handle)
		{
			if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
			{
				auto &uart_port = s_com_ports.at(in_handle);
				auto uart_read_thread = [in_handle](const UARTPort &in_uart_port)
				{
					for (;;)
					{
						char buffer[100 + 1]{0};
						const auto response = read(in_handle, (u8 *)buffer, 100, 0);
						if (0 < response.size)
						{
							for (const auto &user_callback : in_uart_port.m_read_callbacks)
							{
								user_callback(in_handle, (const u8 *)buffer, response.size);
							}
						}
					}
				};
				uart_port.m_uart_read_thread = new std::thread{uart_read_thread, uart_port};
				uart_port.m_uart_read_thread->join();
				return eSUCCESS;
			}
			return eFAILED;
		}

		[[nodiscard]] Response read(Handle in_handle, u8 *out_buffer, const size_t in_read_bytes, u32 in_timeout_ms)
		{
			if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
			{
				auto &uart_port = s_com_ports.at(in_handle);
				unsigned long read_bytes = 0;
				if (!ReadFile(uart_port.m_handle, out_buffer, in_read_bytes, &read_bytes, nullptr))
				{
					OMEGA_LOGE("Write filed failed");
					return {eFAILED, 0};
				}
				return {eSUCCESS, read_bytes};
			}
			return {eSUCCESS, 0};
		}

		[[nodiscard]] Response write(Handle in_handle, const u8 *in_buffer, const size_t in_write_bytes, u32 in_timeout_ms)
		{
			if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
			{
				auto &uart_port = s_com_ports.at(in_handle);
				unsigned long written_bytes = 0;
				if (!WriteFile(uart_port.m_handle, in_buffer, in_write_bytes, &written_bytes, 0))
				{
					OMEGA_LOGE("Write filed failed");
					return {eFAILED, 0};
				}
				return {eSUCCESS, written_bytes};
			}
			return {eSUCCESS, 0};
		}

		OmegaStatus add_on_read_callback(Handle in_handle, std::function<void(const Handle, const u8 *, const size_t)> in_callback)
		{
			if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
			{
				auto &uart_port = s_com_ports.at(in_handle);
				uart_port.m_read_callbacks.push_back(in_callback);
				return eSUCCESS;
			}
			return eFAILED;
		}

		OmegaStatus deinit(const Handle in_handle)
		{
			if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
			{
				auto &uart_port = s_com_ports.at(in_handle);
				uart_port.m_uart_read_thread->join();
				delete uart_port.m_uart_read_thread;
				uart_port.m_uart_read_thread = nullptr;
				if (!CloseHandle(uart_port.m_handle))
				{
					OMEGA_LOGE("Closing COMPORT failed");
					return eFAILED;
				}
				s_com_ports.erase(in_handle);
				return eSUCCESS;
			}
			return eFAILED;
		}
	}
}
