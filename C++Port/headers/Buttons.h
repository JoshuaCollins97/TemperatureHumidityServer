#pragma once
#include <gpiod.h>

/*
Class Buttons accesable through Buttons:: public member functions InputReceived which returns a boolean

*/
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
    