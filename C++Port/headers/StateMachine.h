//State Machine Port from Python CS-350 Project in C++ with additions
#pragma once
//Includes

#include "GPIOInterface.h"
#include "Sensor.h"

enum class State {
    off,
    heat,
    cool

};
class StateMachine {
    public:
        StateMachine(LEDControl& ledsRef, HardwareDisplay& displayRef, Sensor& sensorRef)
        : leds(ledsRef), display(displayRef), sensor(sensorRef),
        currentState(State::off), setPoint(72), alternatingCounter(1) {}

        void Cycle(){
                    switch(currentState) {
                        case State::off: currentState = State::heat; break;
                        case State::cool: currentState = State::off; break;
                        case State::heat: currentState = State::cool; break;
                    }
            }
            //StateMachine controls SetPoint variable, functions increase or decrement.
            void IncreaseSetPoint() { setPoint++;}
            void DecreaseSetPoint() { setPoint--;}
            //Sensor reading input controls what is displayed by the LEDs and LCD.
            void Update(const Sensor::RefinedReadings& data) {
                UpdateDisplay(data);
                UpdateLEDs(data);
            }

            State getState() const { return currentState; }
            uint8_t getSetPoint() { return static_cast<uint8_t>(setPoint); }
        
    private:
    LEDControl& leds;
    HardwareDisplay& display;
    Sensor& sensor;
    State currentState;
    int setPoint;
    int alternatingCounter;
    int displayTimer = 0;
    

    //!!! UPDATE LIGHTS
void UpdateLEDs(const Sensor::RefinedReadings& data) {
        // Convert temperature from tenths (e.g., 725 -> 72.5) for comparison
        float currentTemp = static_cast<float>(data.Temperature) / 10.0f;

        switch (currentState) {
            case State::off:
                leds.RedOn(false);
                leds.BlueOn(false);
                break;

            case State::heat:
                leds.BlueOn(false);
                // Turn on Red LED if current temperature is below setpoint
                if (currentTemp < setPoint) {
                    leds.Pulse(LEDControl::redLEDLine);
                } else {
                    //if current temperature is not below setpoint, then it is at or above. Heat done. LED on.
                    leds.RedOn(true);
                }
                break;

            case State::cool:
                leds.RedOn(false);
                // Pulse Blue LED if current temperature is above setpoint
                if (currentTemp > setPoint) {
                    leds.Pulse(LEDControl::blueLEDLine);
                } else {
                    //If current temperature isn't above, then its below or equal. Cooling is done, set on.
                    leds.BlueOn(true);
                }
                break;
        }
    }

    //!!!!
    void UpdateDisplay(const Sensor::RefinedReadings& data) {
        //Get Time
        std::time_t now = std::time(nullptr);
        std::tm* localTime = std::localtime(&now);
        //Set line 1
        std::ostringstream line1;
        line1 << std::put_time(localTime, "%m/%d %H:%M:%S");

        std::ostringstream line2;
        //Alternator Port for line 2
        static auto lastSwitch = std::chrono::steady_clock::now();
        auto currentTime = std::chrono::steady_clock::now();
        auto elapstedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastSwitch).count();

        if (elapstedTime >= 5) {
            lastSwitch = currentTime;
            alternatingCounter = (alternatingCounter == 1) ? 2 : 1;
            if (alternatingCounter > 2) {
                alternatingCounter = 1;
            }
        }

        if (alternatingCounter == 1) {
            float fahrenheit = static_cast<float>(data.Temperature) / 10.0f;
            line2 << "Temp: " << std::fixed << std::setprecision(2) << fahrenheit << " F";
        } else {
            std::string stateStr;
            //Strings are switched with state, likely issue with LED Pulse function, stateStr swap here results in correct functioning
            //Despite functioning correctly, this is confusing and should be fixed
            switch(currentState) {
                case State::off: stateStr = "Off"; break;
                case State::heat: stateStr = "Cool"; break;
                case State::cool: stateStr = "Heat"; break;
            }
            line2 << stateStr << " SP: " << setPoint << " F";
            }

        //Debug lines
        std::cout << "[LCD Line 1]: " << line1.str() << "\n";
        std::cout << "[LCD Line 2]: " << line2.str() << "\n";
        std::cout << "--------------------\n";
        //Write lines
        display.WriteCommand(0x80);
        display.WriteString(line1.str());

        display.WriteCommand(0xC0);
        display.WriteString(line2.str());
    }

};

