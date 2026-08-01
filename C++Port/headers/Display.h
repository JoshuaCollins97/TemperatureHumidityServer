#pragma once
#include <gpiod.h>


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