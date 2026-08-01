#pragma once
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstdint>

//Temperatures and Humidty sensor controller

class Sensor {
        public:
        struct RefinedReadings {
                //Scale float value by ten, cheaper.
                int16_t Humidity = 0;
                int16_t Temperature = 0;
        };
        Sensor(){}
        ~Sensor(){
                if (i2c >= 0) {
                close(i2c);
        }
    }

        //Open i2c, check if open, check if address is reachable
        void OpenSensor() {
                i2c = open("/dev/i2c-1", O_RDWR);

                if (i2c < 0) {std::cerr << "Sensor File not Open\n"; return;}
                if (ioctl(i2c, I2C_SLAVE, SENS_I2C_ADDRESS) < 0) {std::cerr << "Sensor Unreachable\n"; return;}

                //Wait for sensor to be ready > 100 ms
                std::this_thread::sleep_for(std::chrono::milliseconds(101));
                //Check calibration
                unsigned char checkCmd = 0x71;
                unsigned char status = 0;
                write(i2c, &checkCmd, 1);
                read(i2c, &status, 1);

                //Calibration check
                if ((status & 0x08) == 0) {
                        std::cout << "Sensor uncalibrated.\n";
                        // Initialization routine required for uncalibrated state per AHT20 specs
                        unsigned char init1[3] = {0xBE, 0x08, 0x00};
                        write(i2c, init1, 3);
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        } else {
                        std::cout << "Sensor Calibrated\n";
                        }
                
                //Initialize sensor
                unsigned char init_cmd[3] = {0xBE, 0x08, 0x00};
                write(i2c, init_cmd, 3);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        //Send trigger command. 3 char array containing Trigger command, 0x33, then 0x00.

        RefinedReadings DelayedReading() {
                RefinedReadings currentReading;
                if (i2c < 0) return currentReading;

                unsigned char trigger[3] = {SENS_TRIGGER, SENS_COMMAND_BIT_1, SENS_COMMAND_BIT_2 };
                write(i2c, trigger, 3);
                //Pause for read time
                std::this_thread::sleep_for(std::chrono::milliseconds(80));
                //Create array to hold read return, read, send to array
                unsigned char data[7];
                read(i2c,data, 7);
                //Render Reading !!!! Change variable names
                unsigned long rawHumidity = ((unsigned long)data[1] << 12) | ((unsigned long)data[2] <<4) | (data[3] >> 4);
                currentReading.Humidity = (rawHumidity * 1000) / resolver;

                unsigned long rawTemperature = (((unsigned long)(data[3] & 0x0F)) << 16) | ((unsigned long)data[4] << 8) | data[5];
                float celsius = ((static_cast<float>(rawTemperature) * 200.0f ) / resolver) - 50.0f;
                float fahrenheit = (celsius * 9.0f / 5.0f) + 32.0f;
                currentReading.Temperature = static_cast<int16_t>(fahrenheit * 10.0f);

                return currentReading;
        }
        private:
        int i2c = -1;
        int resolver = 1048576;
        //Static physical addresses for control and command
        //Addresses Listed At: https://cdn-learn.adafruit.com/assets/assets/000/123/394/original/Data_Sheet_AHT20.pdf
        static constexpr uint8_t SENS_I2C_ADDRESS = 0x38;
        //static constexpr uint8_t SENS_CALIBRATION = 0xE1;
        static constexpr uint8_t SENS_COMMAND_BIT_1 = 0x33;
        static constexpr uint8_t SENS_COMMAND_BIT_2 = 0x00;
        static constexpr uint8_t SENS_TRIGGER = 0xAC;
        //static constexpr uint8_t SENS_SOFTRESET = 0xBA;
        //static constexpr uint8_t SENS_STATUS_BUSY = 0x80;
        //static constexpr uint8_t SENS_IS_CALIBATRED = 0x08;

};
