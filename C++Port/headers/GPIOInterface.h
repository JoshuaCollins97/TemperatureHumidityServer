//GPIOInterface.h sets up GPIO related components. The LEDs, 4-bit LCD, and buttons.
#pragma once
#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <unordered_map>
#include <gpiod.hpp>

class LEDControl {
    public:
    LEDControl() : chip("/dev/gpiochip0"), request(init_request(chip)) {};
    // Pulse logic, take offset, control PWM.
        void Pulse(uint8_t linePin) {
            using namespace std::chrono;
            auto now = steady_clock::now();

            // Control speed of fading. The larger the value, the slower the fade.
            if (duration_cast<milliseconds>(now - lastStepTime).count() < 100) {
                return;
            }
            lastStepTime = now;

            // Duty Cycle determiner.
            int onTime = minBrightnessDelayMicroSec + (maxBrightnessDelayMicroSec - minBrightnessDelayMicroSec) * pulseStep / totalSteps;
            int offTime = maxBrightnessDelayMicroSec - onTime;

            // On and off times. Sets active, sleeps. Sets off, sleeps.
            request.set_value(linePin, gpiod::line::value::ACTIVE);
            if (onTime > 0) std::this_thread::sleep_for(microseconds(onTime));
            request.set_value(linePin, gpiod::line::value::INACTIVE);
            if (offTime > 0) std::this_thread::sleep_for(microseconds(offTime));

            //In and out fade.
            pulseStep += fadeDirection;
            if (pulseStep >= totalSteps) {
                pulseStep = totalSteps;
                fadeDirection = -1; // Fade out
            } else if (pulseStep <= 0) {
                pulseStep = 0;
                fadeDirection = 1;  // Fade in
            }
        }
    void RedOn(bool state) {
        request.set_value(redLEDLine, state ? gpiod::line::value::ACTIVE : gpiod::line::value::INACTIVE);
    }
    void BlueOn(bool state) {
        request.set_value(blueLEDLine, state ? gpiod::line::value::ACTIVE : gpiod::line::value::INACTIVE);
    };
    //LED Members, GPIO Line Values
    static constexpr uint8_t redLEDLine = 18;
    static constexpr uint8_t blueLEDLine = 23;

    private:
    //libgpiod members
    gpiod::chip chip;
    gpiod::line_request request;

    //PWM MEMBERS FIX !!!
    int totalSteps = 200;
    int pulseStep = 0;
    int fadeDirection = 1;
    int maxBrightnessDelayMicroSec = 10000;
    int minBrightnessDelayMicroSec = 0;
    std::chrono::steady_clock::time_point lastStepTime = std::chrono::steady_clock::now();
    //initial request, config decleration
    static gpiod::line_request init_request(gpiod::chip& c) {

        gpiod::line_settings settings;
        settings.set_direction(gpiod::line::direction::OUTPUT);
        settings.set_output_value(gpiod::line::value::INACTIVE);

        gpiod::line_config line_cfg;
        line_cfg.add_line_settings({redLEDLine, blueLEDLine}, settings);

            gpiod::request_config req_cfg;
            req_cfg.set_consumer("led_control");
        return c.prepare_request()
        .set_request_config(req_cfg)
        .set_line_config(line_cfg)
        .do_request();

    }
};

class HardwareDisplay {
    public:
    HardwareDisplay() : chip("/dev/gpiochip0"), request(init_request(chip)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    //Instruction set from https://cdn.sparkfun.com/assets/9/5/f/7/b/HD44780.pdf
    WriteCommand(0x33); // Initialize 8-bit mode 
    WriteCommand(0x32); // Lock into 4-bit mode 
    WriteCommand(0x28); // Function set: 2 lines, 5x8 dot matrix
    WriteCommand(0x0C); // Display ON, Cursor OFF, Blinking OFF
    WriteCommand(0x06); // Entry mode: Auto-increment cursor
    ClearScreen();      // Clear old display memory/garbage blocks
    }
    ~HardwareDisplay() {}

    void WriteString(const std::string& StringToDisplay) {
        for (char c : StringToDisplay ) {
            SendByte(static_cast<uint8_t>(c), true);
        }
    }
    //SendByte, send command and low RS signal
    void WriteCommand(uint8_t cmd) {
        SendByte(cmd, false);
    }
    void ClearScreen() {
        WriteCommand(CLEAR);
    };

    private:
    //Physical line numbers for LCD
    static constexpr uint8_t REGISTER_SELECT = 17;
    static constexpr uint8_t ENABLE = 27;
    static constexpr uint8_t DATA_4 = 5;
    static constexpr uint8_t DATA_5 = 6;
    static constexpr uint8_t DATA_6 = 13;
    static constexpr uint8_t DATA_7 = 26;
    // libgpiod v2 member variables
    gpiod::chip chip;
    gpiod::line_request request;
    gpiod::line::value_mappings values;
    //Line request configuration
    static gpiod::line_request init_request(gpiod::chip& c) {
        gpiod::line_settings settings;
        settings.set_direction(gpiod::line::direction::OUTPUT);
        settings.set_output_value(gpiod::line::value::INACTIVE); 

        gpiod::line_config line_cfg;
        line_cfg.add_line_settings({REGISTER_SELECT, ENABLE, DATA_4, DATA_5, DATA_6, DATA_7}, settings);

        gpiod::request_config req_cfg;
        req_cfg.set_consumer("lcd_display");
        
        return c.prepare_request()
        .set_request_config(req_cfg)
        .set_line_config(line_cfg)
        .do_request();    }


    void Pulse() {
        //Pulse enable pin, give it time to register high
       request.set_value(ENABLE, gpiod::line::value::ACTIVE);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        request.set_value(ENABLE, gpiod::line::value::INACTIVE);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));

    }
    void NibbleSetter(uint8_t v) {
        //Value mappings used to prevent race conditions.
        values = {
            {DATA_4, (v & 0x01) ? gpiod::line::value::ACTIVE : gpiod::line::value::INACTIVE},
            {DATA_5, (v & 0x02) ? gpiod::line::value::ACTIVE : gpiod::line::value::INACTIVE},
            {DATA_6, (v & 0x04) ? gpiod::line::value::ACTIVE : gpiod::line::value::INACTIVE},
            {DATA_7, (v & 0x08) ? gpiod::line::value::ACTIVE : gpiod::line::value::INACTIVE}
        };

        request.set_values(values);
        Pulse();
    }

    static constexpr unsigned int CLEAR = 0x01;
    static constexpr unsigned int FUNCTION_ADD = 0x28;
    static constexpr unsigned int CGRAM_ADDR = 0x01;
    static constexpr unsigned int DDRAM_ADDRESS = 0xC0;
    //LCD 16x2 setup variables
    static constexpr uint8_t columns = 16;
    static constexpr uint8_t rows = 2;
    void SendByte(uint8_t byte, bool is_data) {
    //Sending data or command?
        request.set_value(REGISTER_SELECT, is_data ? gpiod::line::value::ACTIVE : gpiod::line::value::INACTIVE);

        //Send high nibble
        NibbleSetter(byte >> 4);

        //Send low nibble
        NibbleSetter(byte & 0x0F);    };
};
//!!!!
class Buttons {
    public:
    Buttons() : chip("/dev/gpiochip0"), request(init_request(chip)) {
        lastState[blueButtonLine] = gpiod::line::value::INACTIVE;
        lastState[redButtonLine] = gpiod::line::value::INACTIVE;
        lastState[greenButtonLine] = gpiod::line::value::INACTIVE;
    }
    ~Buttons() {}

    bool InputReceived(uint8_t buttonLine) {
        gpiod::line::value currentState = request.get_value(buttonLine);
        gpiod::line::value previous_state = lastState[buttonLine];

        // State Update
        lastState[buttonLine] = currentState;

        // Pull down state. Find rising edge.
        if (previous_state == gpiod::line::value::INACTIVE && currentState == gpiod::line::value::ACTIVE) {
            std::this_thread::sleep_for(std::chrono::milliseconds(15)); // Short debounce
            if (request.get_value(buttonLine) == gpiod::line::value::ACTIVE) {
                return true;
            }
        }

        return false;
    }

    //Pin attachments
    static constexpr uint8_t blueButtonLine = 24;
    static constexpr uint8_t redButtonLine = 25;
    static constexpr uint8_t greenButtonLine = 12;

    private:
    std::unordered_map<uint8_t, gpiod::line::value> lastState;
    
    // libgpiod v2 member variables
    gpiod::chip chip;
    gpiod::line_request request;
    
    static gpiod::line_request init_request(gpiod::chip& c) {
        gpiod::line_settings settings;
        settings.set_direction(gpiod::line::direction::INPUT);
        settings.set_bias(gpiod::line::bias::PULL_DOWN); // Keeps input stable when unpressed

        gpiod::line_config line_cfg;
        line_cfg.add_line_settings({blueButtonLine, redButtonLine, greenButtonLine}, settings);

        gpiod::request_config req_cfg;
        req_cfg.set_consumer("button_control");

        return c.prepare_request()
            .set_request_config(req_cfg)
            .set_line_config(line_cfg)
            .do_request();
    }
};
    