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
 * Last Modified: Monday, 12th May 2025 12:03:22 pm
 * Modified By: Omegaki113r (omegaki113r@gmail.com)
 * -----
 * Copyright 2024 - 2025 0m3g4ki113r, Xtronic
 * -----
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	---------------------------------------------------------
 */

#include <atomic>
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

		enum class UARTStatus
		{
			eINITED,
			eCONNECTED,
			eSTARTED,
			eSTOPPED,
			eDISCONNECTED,
			eDEINITED,
		};

		struct UARTPort
		{
			HANDLE m_handle;
			char m_port_name[PORT_NAME_SIZE + 1]{0};
			Baudrate m_baudrate{115200};
			DataBits m_databits{DataBits::eDATA_BITS_8};
			StopBits m_stopbits{StopBits::eSTOP_BITS_1};
			Parity m_parity{Parity::ePARITY_DISABLE};
			std::function<void()> m_connected_callback;
			std::function<void(const Handle, const u8 *, const size_t)> m_read_callbacks;
			std::function<void()> m_disconnected_callback;
			UARTStatus m_status{UARTStatus::eDEINITED};
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
			serial_port.m_status = UARTStatus::eINITED;
			UNUSED(s_com_ports.insert({user_serial_handle, serial_port}));

			return user_serial_handle;
		}

		[[nodiscard]] OmegaStatus connect(Handle in_handle)
		{
			if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
			{
				auto &uart_port = s_com_ports.at(in_handle);

				DCB dcb_parameters{};
				if (!GetCommState(uart_port.m_handle, &dcb_parameters))
				{
					OMEGA_LOGE("Retrieving COMPORT state failed");
					deinit(in_handle);
					return eFAILED;
				}
				dcb_parameters.BaudRate = uart_port.m_baudrate;
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
				dcb_parameters.ByteSize = static_cast<BYTE>(uart_port.m_databits);
				dcb_parameters.StopBits = static_cast<BYTE>(uart_port.m_stopbits);
				dcb_parameters.Parity = static_cast<BYTE>(uart_port.m_parity);
				if (!SetCommState(uart_port.m_handle, &dcb_parameters))
				{
					OMEGA_LOGE("Setting COMPORT state failed");
					deinit(in_handle);
					return eFAILED;
				}
				COMMTIMEOUTS timeout_parameters{};
				/* START: READ TIMEOUT PARAMETERS */
				timeout_parameters.ReadIntervalTimeout = MAXDWORD;
				timeout_parameters.ReadTotalTimeoutConstant = 0x00;
				timeout_parameters.ReadTotalTimeoutMultiplier = 0x00;
				/* END: READ TIMEOUT PARAMETERS */

				/* START: WRITE TIMEOUT PARAMETERS */
				timeout_parameters.WriteTotalTimeoutConstant = 0x00;
				timeout_parameters.WriteTotalTimeoutMultiplier = 0x00;
				/* END: WRITE TIMEOUT PARAMETERS */
				if (!SetCommTimeouts(uart_port.m_handle, &timeout_parameters))
				{
					OMEGA_LOGE("Setting timeouts failed");
					deinit(in_handle);
					return eFAILED;
				}
				uart_port.m_status = UARTStatus::eCONNECTED;
				if (nullptr != uart_port.m_connected_callback)
					uart_port.m_connected_callback();
				return eSUCCESS;
			}
			return eFAILED;
		}

		bool is_connected(Handle in_handle)
		{
			if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
			{
				const auto &uart_port = s_com_ports.at(in_handle);
				return UARTStatus::eSTARTED == uart_port.m_status || UARTStatus::eCONNECTED == uart_port.m_status || UARTStatus::eSTOPPED == uart_port.m_status;
			}
			return false;
		}

		OmegaStatus start(Handle in_handle, const std::function<void(const Handle, const u8 *, const size_t)> &in_callback)
		{
			if (nullptr == in_callback)
				return eFAILED;
			if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
			{
				auto &uart_port = s_com_ports.at(in_handle);
				auto uart_read_thread = [in_handle, in_callback](const UARTPort &in_uart_port)
				{
					while (UARTStatus::eSTARTED == in_uart_port.m_status)
					{
						char buffer[100 + 1]{0};
						const auto response = read(in_handle, (u8 *)buffer, 100, 0);
						if (0 < response.size)
						{
							in_callback(in_handle, (const u8 *)buffer, response.size);
						}
					}
				};
				uart_port.m_uart_read_thread = new std::thread{uart_read_thread, std::ref(uart_port)};
				uart_port.m_status = UARTStatus::eSTARTED;
				return eSUCCESS;
			}
			return eFAILED;
		}

		[[nodiscard]] Response read(Handle in_handle, u8 *out_buffer, const size_t in_read_bytes, u32 in_timeout_ms)
		{
			if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
			{
				auto &uart_port = s_com_ports.at(in_handle);
				if (UARTStatus::eCONNECTED != uart_port.m_status && UARTStatus::eSTARTED != uart_port.m_status)
				{
					OMEGA_LOGE("UART is in Invalid state");
					return {eFAILED, 0};
				}
				COMMTIMEOUTS timeout_parameters{};
				if (!GetCommTimeouts(uart_port.m_handle, &timeout_parameters))
				{
					OMEGA_LOGE("Timeout retrieval failed");
					return {eFAILED, 0};
				}
				timeout_parameters.ReadTotalTimeoutConstant = in_timeout_ms;
				if (!SetCommTimeouts(uart_port.m_handle, &timeout_parameters))
				{
					OMEGA_LOGE("Timeout setting failed");
					return {eFAILED, 0};
				}
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
				if (UARTStatus::eCONNECTED != uart_port.m_status && UARTStatus::eSTARTED != uart_port.m_status)
				{
					return {eFAILED, 0};
				}
				COMMTIMEOUTS timeout_parameters{};
				if (!GetCommTimeouts(uart_port.m_handle, &timeout_parameters))
				{
					OMEGA_LOGE("Timeout retrieval failed");
					return {eFAILED, 0};
				}
				timeout_parameters.WriteTotalTimeoutConstant = in_timeout_ms;
				if (!SetCommTimeouts(uart_port.m_handle, &timeout_parameters))
				{
					OMEGA_LOGE("Timeout setting failed");
					return {eFAILED, 0};
				}
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

		Handle change_baudrate(Handle in_handle, Baudrate baudrate)
		{
			if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
			{
				auto uart_port = s_com_ports.at(in_handle);
				if (const auto state = stop(in_handle); eSUCCESS != state)
				{
					OMEGA_LOGE("Stopping UART failed");
					return INVALID_UART_HANDLE;
				}
				if (const auto state = disconnect(in_handle); eSUCCESS != state)
				{
					OMEGA_LOGE("Disconnecting UART failed");
					return INVALID_UART_HANDLE;
				}
				if (const auto state = deinit(in_handle); eSUCCESS != state)
				{
					OMEGA_LOGE("Deiniting UART failed");
					return INVALID_UART_HANDLE;
				}

				const auto new_handle = init(uart_port.m_port_name, baudrate, uart_port.m_databits, uart_port.m_parity, uart_port.m_stopbits);
				if (INVALID_UART_HANDLE == new_handle)
				{
					OMEGA_LOGE("Initializing UART failed");
					return INVALID_UART_HANDLE;
				}
				if (const auto state = connect(new_handle); eSUCCESS != state)
				{
					OMEGA_LOGE("Connecting to UART failed");
					return INVALID_UART_HANDLE;
				}
				if (nullptr != uart_port.m_connected_callback)
				{
					add_on_connected_callback(new_handle, uart_port.m_connected_callback);
				}
				if (nullptr != uart_port.m_disconnected_callback)
				{
					add_on_disconnected_callback(new_handle, uart_port.m_disconnected_callback);
				}
				if (nullptr != uart_port.m_read_callbacks)
				{
					if (const auto state = start(new_handle, uart_port.m_read_callbacks); eSUCCESS != state)
					{
						OMEGA_LOGE("Starting UART failed");
					}
				}
				return new_handle;
			}
			return INVALID_UART_HANDLE;
		}

		Configuration get_configuration(Handle in_handle)
		{
			if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
			{
				auto &uart_port = s_com_ports.at(in_handle);
				return {uart_port.m_baudrate, uart_port.m_databits, uart_port.m_parity, uart_port.m_stopbits};
			}
			return {};
		}

		void set_configuration(Handle in_handle, const Configuration &in_configuration)
		{
			if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
			{
				auto &uart_port = s_com_ports.at(in_handle);
				uart_port.m_baudrate = in_configuration.baudrate;
				uart_port.m_databits = in_configuration.databits;
				uart_port.m_parity = in_configuration.parity;
				uart_port.m_stopbits = in_configuration.stopbits;

				const auto &status = uart_port.m_status;
				if (UARTStatus::eSTARTED == status)
				{
					if (const auto err = stop(in_handle); eSUCCESS != err)
					{
						OMEGA_LOGE("Stopping failed");
						return;
					}
				}
			}
		}

		OmegaStatus stop(Handle in_handle)
		{
			if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
			{
				auto &uart_port = s_com_ports.at(in_handle);
				if (nullptr != uart_port.m_uart_read_thread)
				{
					uart_port.m_status = UARTStatus::eSTOPPED;
					uart_port.m_uart_read_thread->join();
					delete uart_port.m_uart_read_thread;
					uart_port.m_uart_read_thread = nullptr;
					return eSUCCESS;
				}
			}
			return eFAILED;
		}

		OmegaStatus disconnect(Handle in_handle)
		{
			if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
			{
				auto &uart_port = s_com_ports.at(in_handle);
				if (nullptr != uart_port.m_uart_read_thread)
				{
					uart_port.m_status = UARTStatus::eSTOPPED;
					uart_port.m_uart_read_thread->join();
					delete uart_port.m_uart_read_thread;
					uart_port.m_uart_read_thread = nullptr;
				}
				if (!CloseHandle(uart_port.m_handle))
				{
					OMEGA_LOGE("Closing COMPORT failed");
					return eFAILED;
				}
				uart_port.m_status = UARTStatus::eDISCONNECTED;
				if (nullptr != uart_port.m_disconnected_callback)
					uart_port.m_disconnected_callback();
				return eSUCCESS;
			}
			return eFAILED;
		}

		OmegaStatus add_on_connected_callback(Handle in_handle, std::function<void()> in_callback)
		{
			if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
			{
				auto &uart_port = s_com_ports.at(in_handle);
				uart_port.m_connected_callback = in_callback;
				return eSUCCESS;
			}
			return eFAILED;
		}

		OmegaStatus add_on_disconnected_callback(Handle in_handle, std::function<void()> in_callback)
		{
			if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
			{
				auto &uart_port = s_com_ports.at(in_handle);
				uart_port.m_disconnected_callback = in_callback;
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
					uart_port.m_status = UARTStatus::eSTOPPED;
					uart_port.m_uart_read_thread->join();
					delete uart_port.m_uart_read_thread;
					uart_port.m_uart_read_thread = nullptr;
				}
				if (UARTStatus::eDISCONNECTED == uart_port.m_status || UARTStatus::eDEINITED == uart_port.m_status)
				{
					return eSUCCESS;
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
