#include <iostream>
#include <thread>
#include <chrono>

//Original Includes
#include "headers/GPIOInterface.h"
#include "headers/Sensor.h"
#include "headers/StateMachine.h"
#include "headers/Serial.h"



int main() {
    std::cout << "Initiating Thermostat...\n";

    //Hardware Initialization
    
    LEDControl LEDS;
    HardwareDisplay HardwareDisplay;
    Buttons Buttons;
    Sensor AHT20;
    Serial serial; 
    StateMachine stateMachine(LEDS, HardwareDisplay, AHT20);
    AHT20.OpenSensor();
    HardwareDisplay.ClearScreen();

    //Poll sensor once, then again every 30 seconds. Store current reading in currentSensorData
    auto lastSensorPoll = std::chrono::steady_clock::time_point::min();
    const auto sensor_poll_interval = std::chrono::seconds(30);
    Sensor::RefinedReadings currentSensorData = {0, 0}; // Cached data holder


    bool alreadyRead = false;

    //Main Operation Loop
while (true) {
        auto now = std::chrono::steady_clock::now();

        // Force a read immediately on the first cycle, then every 30 seconds
        if (!alreadyRead || std::chrono::duration_cast<std::chrono::seconds>(now - lastSensorPoll) >= sensor_poll_interval) {
            lastSensorPoll = now;
            currentSensorData = AHT20.DelayedReading(); 
            alreadyRead = true;
        }

        //Button input
        if (Buttons.InputReceived(Buttons::greenButtonLine)) {
            stateMachine.Cycle();
        }
        if (Buttons.InputReceived(Buttons::blueButtonLine)) {
            stateMachine.DecreaseSetPoint();
        }
        if (Buttons.InputReceived(Buttons::redButtonLine)) {
            stateMachine.IncreaseSetPoint();
        }

        stateMachine.Update(currentSensorData);

        //Send serial string
        std::string payload = serial.setupSerialOutput(static_cast<int>(stateMachine.getState()), currentSensorData.Temperature, stateMachine.getSetPoint());
        serial.SendData(payload);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    return 0;
}
