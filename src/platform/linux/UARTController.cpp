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
 * Last Modified: Wednesday, 23rd April 2025 1:47:49 am
 * Modified By: 0m3g4ki113r (omegaki113r@gmail.com)
 * -----
 * Copyright 2024 - 2025 0m3g4ki113r, Xtronic
 * -----
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	---------------------------------------------------------
 */

#include <stdio.h>
#include <dirent.h>
#include <cstring>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#include "OmegaUtilityDriver/UtilityDriver.hpp"
#include "OmegaUARTController/UARTController.hpp"

struct TermiosBaudrates
{
    const u32 m_user_baurdare;
    const u16 m_termios_baudrate;
};

const size_t TERMIOS_BAUDRATE_COUNT = 30;
const TermiosBaudrates termios_baudrates[30] = {
    {50, B50},
    {75, B75},
    {110, B110},
    {134, B134},
    {150, B150},
    {200, B200},
    {300, B300},
    {600, B600},
    {1200, B1200},
    {1800, B1800},
    {2400, B2400},
    {4800, B4800},
    {9600, B9600},
    {19200, B19200},
    {38400, B38400},
    {57600, B57600},
    {115200, B115200},
    {230400, B230400},
    {460800, B460800},
    {500000, B500000},
    {576000, B576000},
    {921600, B921600},
    {1000000, B1000000},
    {1152000, B1152000},
    {1500000, B1500000},
    {2000000, B2000000},
    {2500000, B2500000},
    {3000000, B3000000},
    {3500000, B3500000},
    {4000000, B4000000},
};

namespace Omega
{
    namespace UART
    {

        struct UARTPort
        {
            int m_handle;
            char m_port_name[PORT_NAME_SIZE + 1]{0};
            Baudrate m_baudrate{115200};
            DataBits m_databits{DataBits::eDATA_BITS_8};
            StopBits m_stopbits{StopBits::eSTOP_BITS_1};
            Parity m_parity{Parity::ePARITY_DISABLE};
            std::vector<std::function<void(const Handle, const u8 *, const size_t)>> m_read_callbacks;
            std::thread *m_uart_read_thread{nullptr};
        };
        __internal__ std::unordered_map<Handle, UARTPort> s_com_ports;

        std::vector<EnumeratedUARTPort> get_available_ports()
        {
            std::vector<EnumeratedUARTPort> serial_ports;

            DIR *dir;
            struct dirent *entry;
            const char *dev_dir = "/dev";
            const char *patterns[] = {"ttyS", "ttyUSB", "ttyACM"};
            int i;
            dir = opendir(dev_dir);
            if (dir == NULL)
            {
                OMEGA_LOGE("Unable to open /dev");
                return serial_ports;
            }
            while ((entry = readdir(dir)) != NULL)
            {
                for (i = 0; i < sizeof(patterns) / sizeof(patterns[0]); i++)
                {
                    if (strncmp(entry->d_name, patterns[i], strlen(patterns[i])) == 0)
                    {
                        EnumeratedUARTPort uart_port{0};
                        UNUSED(sprintf(uart_port.m_port_name, "/dev/%s", entry->d_name));
                        serial_ports.push_back(uart_port);
                    }
                }
            }
            closedir(dir);
            return serial_ports;
        }

        Handle init(const char *in_port, Baudrate in_baudrate, DataBits in_databits, Parity in_parity, StopBits in_stopbits)
        {
            __internal__ Handle user_serial_handle = 0;

            if (nullptr == in_port || 0 == std::strlen(in_port))
            {
                OMEGA_LOGE("Invalid serial port path");
                return 0;
            }

            int termios_baudrate_index = -1;
            for (size_t idx = 0; idx < TERMIOS_BAUDRATE_COUNT; ++idx)
            {
                if (in_baudrate == termios_baudrates[idx].m_user_baurdare)
                {
                    termios_baudrate_index = idx;
                    break;
                }
            }
            if (-1 == termios_baudrate_index)
            {
                OMEGA_LOGE("Custom baudrates are not supported by Linux");
                return 0;
            }

            int serial_handle = 0;
            if (serial_handle = open(in_port, O_RDWR | O_NOCTTY); -1 == serial_handle)
            {
                OMEGA_LOGE("Opening serial port failed with %s", strerror(errno));
                return 0;
            }

            struct termios termios_config{};
            if (tcgetattr(serial_handle, &termios_config) != 0)
            {
                OMEGA_LOGE("tcgetattr");
                return 0;
            }
            if (const auto status = cfsetospeed(&termios_config, termios_baudrates[termios_baudrate_index].m_termios_baudrate); -1 == status)
            {
                OMEGA_LOGE("cfsetospeed failed for %d", in_baudrate);
                return 0;
            }
            if (const auto status = cfsetispeed(&termios_config, termios_baudrates[termios_baudrate_index].m_termios_baudrate); -1 == status)
            {
                OMEGA_LOGE("cfsetispeed failed for %d", in_baudrate);
                return 0;
            }

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

            tcflush(serial_handle, TCIOFLUSH);

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

        OmegaStatus deinit(const Handle in_handle)
        {
            if (const auto found = s_com_ports.find(in_handle); s_com_ports.end() != found)
            {
                auto &uart_port = s_com_ports.at(in_handle);
                uart_port.m_uart_read_thread->join();
                delete uart_port.m_uart_read_thread;
                uart_port.m_uart_read_thread = nullptr;
                tcflush(uart_port.m_handle, TCIOFLUSH);
                close(uart_port.m_handle);
                s_com_ports.erase(in_handle);
                return eSUCCESS;
            }
            return eFAILED;
        }
    } // namespace UART
} // namespace Omega
