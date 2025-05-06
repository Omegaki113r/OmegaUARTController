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
 * Last Modified: Saturday, 26th April 2025 3:11:10 pm
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

#include <cfgmgr32.h>
#include <setupapi.h>
#include <tchar.h>

#include <initguid.h>

#include "OmegaUARTController/UARTController.hpp"
#include "OmegaUtilityDriver/UtilityDriver.hpp"

namespace Omega
{
	namespace UART
	{
		DEFINE_GUID(GUID_DEVINTERFACE_COMPORT, 0x86E0D1E0, 0x8089, 0x11D0, 0x9C, 0xE4, 0x08, 0x00, 0x3E, 0x30, 0x1F, 0x73);

		struct UARTPort
		{
			HANDLE m_handle;
			char m_port_name[PORT_NAME_SIZE + 1]{0};
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
			char deviceName[PORT_NAME_SIZE + 1]{0};
			sprintf(deviceName, "\\\\.\\%s", in_port);
			if (serial_handle = CreateFile(deviceName, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr); INVALID_HANDLE_VALUE == serial_handle)
			{
				OMEGA_LOGE("Opening Serialport failed. Reason: %s", ERROR_FILE_NOT_FOUND == GetLastError() ? "COMPORT NOT FOUND" : "OPENING COMPORT FAILED");
				return 0;
			}
			user_serial_handle++;
			UARTPort serial_port{
				.m_handle = serial_handle,
				.m_baudrate = in_baudrate,
				.m_databits = in_databits,
				.m_stopbits = in_stopbits,
				.m_parity = in_parity,
			};
			UNUSED(std::strncpy(serial_port.m_port_name, in_port, PORT_NAME_SIZE));
			UNUSED(s_com_ports.insert({user_serial_handle, serial_port}));

			DCB dcb_parameters{};
			if (!GetCommState(serial_handle, &dcb_parameters))
			{
				OMEGA_LOGE("Retrieving COMPORT state failed");
				deinit(user_serial_handle);
				return 0;
			}
			dcb_parameters.BaudRate = in_baudrate;
			dcb_parameters.fBinary = TRUE;
			dcb_parameters.fParity = FALSE;
			dcb_parameters.fOutxCtsFlow = FALSE;
			dcb_parameters.fOutxDsrFlow = FALSE;
			dcb_parameters.fDtrControl = DTR_CONTROL_DISABLE;
			dcb_parameters.fDsrSensitivity = FALSE;
			dcb_parameters.fTXContinueOnXoff = TRUE;
			dcb_parameters.fOutX = FALSE;
			dcb_parameters.fInX = FALSE;
			dcb_parameters.fErrorChar = FALSE;
			dcb_parameters.fNull = FALSE;
			dcb_parameters.fRtsControl = RTS_CONTROL_DISABLE;
			dcb_parameters.fRtsControl = RTS_CONTROL_DISABLE;
			dcb_parameters.fAbortOnError = FALSE;
			dcb_parameters.ByteSize = static_cast<BYTE>(in_databits);
			dcb_parameters.StopBits = static_cast<BYTE>(in_stopbits);
			dcb_parameters.Parity = static_cast<BYTE>(in_parity);
			if (!SetCommState(serial_handle, &dcb_parameters))
			{
				OMEGA_LOGE("Setting COMPORT state failed");
				deinit(user_serial_handle);
				return 0;
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
				return 0;
			}
			return user_serial_handle;
		}

		bool connected(Handle in_handle)
		{
			if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
			{
				return true;
			}
			return false;
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
				return eSUCCESS;
			}
			return eFAILED;
		}

		[[nodiscard]] Response read(Handle in_handle, u8 *out_buffer, const size_t in_read_bytes, u32 in_timeout_ms)
		{
		    if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
		    {
		        auto &uart_port = s_com_ports.at(in_handle);
		        OVERLAPPED ov = {};
		        ov.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);  // manual reset
		
		        DWORD read_bytes = 0;
		        BOOL result = ReadFile(uart_port.m_handle, out_buffer, in_read_bytes, &read_bytes, &ov);
		        if (!result)
		        {
		            if (GetLastError() == ERROR_IO_PENDING)
		            {
		                DWORD wait_result = WaitForSingleObject(ov.hEvent, in_timeout_ms);
		                if (wait_result == WAIT_OBJECT_0)
		                {
		                    GetOverlappedResult(uart_port.m_handle, &ov, &read_bytes, FALSE);
		                }
		                else
		                {
		                    CancelIo(uart_port.m_handle); // Timeout occurred
		                    CloseHandle(ov.hEvent);
		                    return {eFAILED, 0};
		                }
		            }
		            else
		            {
		                CloseHandle(ov.hEvent);
		                OMEGA_LOGE("ReadFile failed");
		                return {eFAILED, 0};
		            }
		        }
		
		        CloseHandle(ov.hEvent);
		        return {eSUCCESS, read_bytes};
		    }
		    return {eFAILED, 0};
		}


		[[nodiscard]] Response write(Handle in_handle, const u8 *in_buffer, const size_t in_write_bytes, u32 in_timeout_ms)
		{
		    if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
		    {
		        auto &uart_port = s_com_ports.at(in_handle);
		        OVERLAPPED ov = {};
		        ov.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		
		        DWORD written_bytes = 0;
		        BOOL result = WriteFile(uart_port.m_handle, in_buffer, in_write_bytes, &written_bytes, &ov);
		        if (!result)
		        {
		            if (GetLastError() == ERROR_IO_PENDING)
		            {
		                DWORD wait_result = WaitForSingleObject(ov.hEvent, in_timeout_ms);
		                if (wait_result == WAIT_OBJECT_0)
		                {
		                    GetOverlappedResult(uart_port.m_handle, &ov, &written_bytes, FALSE);
		                }
		                else
		                {
		                    CancelIo(uart_port.m_handle); // Timeout occurred
		                    CloseHandle(ov.hEvent);
		                    return {eFAILED, 0};
		                }
		            }
		            else
		            {
		                CloseHandle(ov.hEvent);
		                OMEGA_LOGE("WriteFile failed");
		                return {eFAILED, 0};
		            }
		        }
		
		        CloseHandle(ov.hEvent);
		        return {eSUCCESS, written_bytes};
		    }
		    return {eFAILED, 0};
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
				if (nullptr != uart_port.m_uart_read_thread)
				{
					uart_port.m_uart_read_thread->join();
					delete uart_port.m_uart_read_thread;
					uart_port.m_uart_read_thread = nullptr;
				}
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

		std::vector<EnumeratedUARTPort> get_available_ports()
		{
			std::vector<EnumeratedUARTPort> enumerated_ports;
			enumerated_ports.reserve(10);

			// Get device information set for COM ports
			HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_COMPORT, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
			if (hDevInfo == INVALID_HANDLE_VALUE)
			{
				OMEGA_LOGE("Error: Unable to get device info set (%lu)", GetLastError());
				return enumerated_ports;
			}

			SP_DEVINFO_DATA devInfoData = {sizeof(SP_DEVINFO_DATA)};
			DWORD index = 0;

			// Enumerate all COM port devices
			while (SetupDiEnumDeviceInfo(hDevInfo, index, &devInfoData))
			{
				TCHAR friendlyName[256] = {0};
				TCHAR portName[32] = {0};
				DWORD size = 0;

				// Get friendly name (e.g., "Communications Port (COM1)")
				if (SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_FRIENDLYNAME, NULL, (PBYTE)friendlyName, sizeof(friendlyName), &size))
				{
					// Get port name from registry
					HKEY hKey = SetupDiOpenDevRegKey(hDevInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
					if (hKey != INVALID_HANDLE_VALUE)
					{
						size = sizeof(portName);
						DWORD type = 0;
						if (RegQueryValueEx(hKey, _T("PortName"), NULL, &type, (LPBYTE)portName, &size) == ERROR_SUCCESS)
						{
							//_tprintf(_T("%s (%s)\n"), friendlyName, portName);
							EnumeratedUARTPort port{};
							UNUSED(std::strncpy(port.m_friendly_portname, friendlyName, FRIENDLY_PORT_NAME_SIZE));
							UNUSED(std::strncpy(port.m_port_name, portName, PORT_NAME_SIZE));
							enumerated_ports.push_back(port);
						}
						RegCloseKey(hKey);
					}
					else
					{
						_tprintf(_T("%s (Port name unavailable)\n"), friendlyName);
					}
				}

				index++;
			}

			if (GetLastError() != ERROR_NO_MORE_ITEMS)
			{
				OMEGA_LOGE("Error enumerating devices (%lu)", GetLastError());
			}

			// Clean up
			SetupDiDestroyDeviceInfoList(hDevInfo);
			return enumerated_ports;
		}
	}
}
