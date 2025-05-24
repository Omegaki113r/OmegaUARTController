#include <cstdio>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "OmegaUARTController/UARTController.hpp"
#include "OmegaUtilityDriver/UtilityDriver.hpp"

namespace Omega
{
    namespace UART
    {
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
            int m_handle;
            char m_port_name[PORT_NAME_SIZE + 1]{0};
            Baudrate m_baudrate{115200};
            DataBits m_databits{DataBits::eDATA_BITS_8};
            StopBits m_stopbits{StopBits::eSTOP_BITS_1};
            Parity m_parity{Parity::ePARITY_DISABLE};
            std::function<void()> m_connected_callback;
			std::function<void(const Handle, const u8 *, const size_t)> m_read_callback;
			std::function<void()> m_disconnected_callback;
            UARTStatus m_status{UARTStatus::eDEINITED};
            std::thread *m_uart_read_thread{nullptr};
        };
        __internal__ std::unordered_map<Handle, UARTPort> s_com_ports;

        std::string CFStringToString(CFStringRef cfString)
        {
            if (!cfString)
            {
                return "";
            }
            CFIndex length = CFStringGetLength(cfString);
            char buffer[256];
            if (CFStringGetCString(cfString, buffer, sizeof(buffer), kCFStringEncodingUTF8))
            {
                return std::string(buffer);
            }
            return "";
        }

        void PrintKernError(kern_return_t result, const std::string &operation)
        {
            if (result != KERN_SUCCESS)
            {
                OMEGA_LOGE("%s failed with error: %d", operation.c_str(), result);
            }
        }

        std::vector<EnumeratedUARTPort> get_available_ports()
        {
            std::vector<EnumeratedUARTPort> uart_ports;

            CFMutableDictionaryRef matchingDict = IOServiceMatching(kIOSerialBSDServiceValue); // "IOSerialBSDClient"
            if (!matchingDict)
            {
                return uart_ports;
            }

            CFDictionarySetValue(matchingDict, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDAllTypes)); // matches both /dev/tty.* and /dev/cu.*

            io_iterator_t iterator;
            kern_return_t kr = IOServiceGetMatchingServices(kIOMainPortDefault, matchingDict, &iterator);
            if (kr != KERN_SUCCESS)
            {
                return uart_ports;
            }

            io_object_t device;
            while ((device = IOIteratorNext(iterator)))
            {
                CFTypeRef calloutPath = IORegistryEntryCreateCFProperty(device, CFSTR(kIOCalloutDeviceKey), kCFAllocatorDefault, 0);
                if (calloutPath && CFGetTypeID(calloutPath) == CFStringGetTypeID())
                {
                    char path[PATH_MAX];
                    if (CFStringGetCString((CFStringRef)calloutPath, path, sizeof(path), kCFStringEncodingUTF8))
                    {
                        EnumeratedUARTPort port{};
                        UNUSED(std::strncpy(port.m_port_name, path, sizeof(port.m_port_name)));
                        uart_ports.push_back(port);
                    }
                }
                if (calloutPath)
                    CFRelease(calloutPath);
                IOObjectRelease(device);
            }

            IOObjectRelease(iterator);
            return uart_ports;
        }

        Handle init(const char *in_port, Baudrate in_baudrate, DataBits in_databits, Parity in_parity, StopBits in_stopbits)
        {
            __internal__ Handle user_serial_handle = 0;

            if (nullptr == in_port || 0 == std::strlen(in_port))
            {
                OMEGA_LOGE("Invalid serial port path");
                return 0;
            }

            int serial_handle = 0;
            OMEGA_LOGD("%s %d", in_port, std::strlen(in_port));
            if (serial_handle = open(in_port, O_RDWR | O_NOCTTY); -1 == serial_handle)
            {
                OMEGA_LOGE("Opening serial port failed with %s", strerror(errno));
                return 0;
            }

            struct termios termios_config{0};
            if (tcgetattr(serial_handle, &termios_config) != 0)
            {
                perror("tcgetattr");
                return false;
            }
            cfsetospeed(&termios_config, in_baudrate);
            cfsetispeed(&termios_config, in_baudrate);

            termios_config.c_cflag &= ~CSIZE;
            switch (in_databits)
            {
            case DataBits::eDATA_BITS_5:
            {
                termios_config.c_cflag |= CS5;
                break;
            }
            case DataBits::eDATA_BITS_6:
            {
                termios_config.c_cflag |= CS6;
                break;
            }
            case DataBits::eDATA_BITS_7:
            {
                termios_config.c_cflag |= CS7;
                break;
            }
            case DataBits::eDATA_BITS_8:
            {
                termios_config.c_cflag |= CS8;
                break;
            }
            }
            switch (in_parity)
            {
            case Parity::ePARITY_DISABLE:
            {
                termios_config.c_cflag &= ~PARENB;
                break;
            }
            case Parity::ePARITY_ODD:
            {
                termios_config.c_cflag |= PARENB;
                termios_config.c_cflag &= ~PARODD;
                break;
            }
            case Parity::ePARITY_EVEN:
            {
                termios_config.c_cflag |= PARENB;
                termios_config.c_cflag |= PARODD;
                break;
            }
            }

            switch (in_stopbits)
            {
            case StopBits::eSTOP_BITS_1:
            {
                termios_config.c_cflag &= ~CSTOPB;
                break;
            }
            case StopBits::eSTOP_BITS_2:
            {
                termios_config.c_cflag |= CSTOPB;
                break;
            }
            }
            // Raw input/output (no canonical mode, no echo, no signal chars)
            termios_config.c_lflag = 0;
            termios_config.c_oflag = 0;
            termios_config.c_iflag &= ~(IXON | IXOFF | IXANY); // Disable software flow control

            // No blocking on read, unless at least 1 byte available
            termios_config.c_cc[VMIN] = 1;
            termios_config.c_cc[VTIME] = 0;

            if (tcsetattr(serial_handle, TCSANOW, &termios_config) != 0)
            {
                perror("tcsetattr");
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

            return user_serial_handle;
        }

        OmegaStatus connect(Handle in_handle)
        {
            OMEGA_LOGE("Not implemented");
            return eFAILED;
        }

        bool is_connected(Handle in_handle)
        {
            if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
            {
                return true;
            }
            return false;
        }

        OmegaStatus start(Handle in_handle,const std::function<void(const Handle, const u8 *, const size_t)> in_callback)
        {
            if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
            {
                auto &uart_port = s_com_ports.at(in_handle);
                uart_port.m_read_callback = in_callback;
                auto uart_read_thread = [in_handle, &uart_port](const UARTPort &in_uart_port)
                {
                    for (;;)
                    {
                        char buffer[100 + 1]{0};
                        const auto response = read(in_handle, (u8 *)buffer, 100, 0);
                        if (0 < response.size)
                        {
                                uart_port.m_read_callback(in_handle, (const u8 *)buffer, response.size);
                        }
                    }
                };
                uart_port.m_uart_read_thread = new std::thread{uart_read_thread, uart_port};
                return eSUCCESS;
            }
            return eFAILED;
        }

        Response read(Handle in_handle, u8 *out_buffer, const size_t in_read_bytes, u32 in_timeout_ms)
        {
            if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
            {
                auto &uart_port = s_com_ports.at(in_handle);
                unsigned long read_bytes = 0;
                if (read_bytes = ::read(uart_port.m_handle, out_buffer, in_read_bytes); -1 == read_bytes)
                {
                    OMEGA_LOGE("read filed failed");
                    return {eFAILED, 0};
                }
                return {eSUCCESS, read_bytes};
            }
            return {eSUCCESS, 0};
        }

        Response write(Handle in_handle, const u8 *in_buffer, const size_t in_write_bytes, u32 in_timeout_ms)
        {
            if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
            {
                auto &uart_port = s_com_ports.at(in_handle);
                unsigned long written_bytes = 0;
                if (written_bytes = ::write(uart_port.m_handle, in_buffer, in_write_bytes); -1 == written_bytes)
                {
                    OMEGA_LOGE("Write filed failed");
                    return {eFAILED, 0};
                }
                return {eSUCCESS, written_bytes};
            }
            return {eSUCCESS, 0};
        }

        Handle change_baudrate(Handle in_handle, Baudrate in_baudrate)
        {
            OMEGA_LOGE("Not implemented!!!");
            return in_handle;
        }

        Configuration get_configuration(Handle in_handle)
        {
            OMEGA_LOGE("Not implemented");
            return {};
        }

        void set_configuration(Handle in_handle, Configuration in_configuration)
        {
            OMEGA_LOGE("Not implemented");
        }

        OmegaStatus stop(Handle in_handle)
        {
            OMEGA_LOGE("Not implemented");
            return eFAILED;
        }

        OmegaStatus disconnect(Handle in_handle)
        {
            OMEGA_LOGE("Not implemented");
            return eFAILED;
        }

        OmegaStatus deinit(Handle in_handle)
        {
            OMEGA_LOGE("Not implemented");
            return eFAILED;
        }

        OmegaStatus add_on_connected_callback(Handle in_handle, std::function<void(void)> in_callback)
        {
            if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
            {
                auto &uart_port = s_com_ports.at(in_handle);
                
                uart_port.m_connected_callback = in_callback;
                return eSUCCESS;
            }
            return eFAILED;
        }

        OmegaStatus add_on_disconnected_callback(Handle in_handle, std::function<void(void)> in_callback)
        {
            if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
            {
                auto &uart_port = s_com_ports.at(in_handle);
                
                uart_port.m_disconnected_callback = in_callback;
                return eSUCCESS;
            }
            return eFAILED;
        }
    } // namespace UART
} // namespace Omega
