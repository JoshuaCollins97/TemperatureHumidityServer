#pragma once
#include <gpiod.h>

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
