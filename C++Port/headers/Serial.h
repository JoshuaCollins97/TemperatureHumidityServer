#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>

class Serial {
public:
    Serial(const char* port = "/dev/ttyS0", int baudRate = B115200) {
        //Open serial port, give read/write access
        serial_fd = open(port, O_RDWR | O_NOCTTY);
        if (serial_fd < 0) {
            std::cerr << "Error opening serial port: " << strerror(errno) << "\n";
            return;
        }

        struct termios options;
        tcgetattr(serial_fd, &options);

        // Set baud rate
        cfsetispeed(&options, baudRate);
        cfsetospeed(&options, baudRate);

        //Configuration
        options.c_cflag &= ~PARENB; // No parity
        options.c_cflag &= ~CSTOPB; // 1 stop bit
        options.c_cflag &= ~CSIZE;  // Clear data size mask
        options.c_cflag |= CS8;     // 8 data bits

        // Disable hardware flow control
        options.c_cflag &= ~CRTSCTS;

        // Enable receiver, make local.
        options.c_cflag |= (CLOCAL | CREAD);

        // Set IO.
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        options.c_oflag &= ~OPOST;

        // Timeout configuration
        options.c_cc[VMIN] = 0;
        options.c_cc[VTIME] = 10; // Deciseconds (10 * 100ms = 1 second)

        // Apply settings
        if (tcsetattr(serial_fd, TCSANOW, &options) != 0) {
            std::cerr << "Error setting serial attributes: " << strerror(errno) << "\n";
        }
    }

    ~Serial() {
        if (serial_fd >= 0) {
            close(serial_fd);
        }
    }

    // Port of python setupSerialOutput method
    std::string setupSerialOutput(int stateId, int16_t temperature, int setPoint) {
        float fahrenheit = static_cast<float>(temperature) / 10.0f;
        
        std::ostringstream output;
        output << stateId << "," << std::fixed << std::setprecision(1) << fahrenheit << "," << setPoint << "\n";        
        return output.str();
    }

    void SendData(const std::string& data) {
        if (serial_fd >= 0) {
            write(serial_fd, data.c_str(), data.length());
        }
    }

private:
    int serial_fd = -1;
};