//Ring Buffer for Temporary Storage of AHT20 Temperature and Humidity Sensor readings
#pragma once
//Needs access to sensor.
#include "Sensor.h"
#include <algorithm>
#include <cmath>
//RingBuffer class, includes anomoly detection methods.
class RingBuffer {

    public:
    struct BadReads {
        bool badTemp = false;
        bool badHumidity = false;
        bool anomaly() const { return badTemp || badHumidity; }

    };

    BadReads Push(const Sensor::RefinedReadings& reading) {
        BadReads result;
        if (writeCount > 0 ) {
            double tempMean = meanTemperature();
            double tempStdDev = stdDevTemperature(tempMean);
            double currentTempF = reading.Temperature / 10.0;
            
            if (tempStdDev > 0.0 &&
            std::abs(currentTempF - tempMean) > samuelsonDerivedThreshold * tempStdDev) {
                result.badTemp = true;
            }

            double humMean = meanHumidity();
            double humStdDev = stdDevHumidity(humMean);
            double currentHumidity = reading.Humidity / 10.0;

            if (humStdDev > 0.0 &&
            std::abs(currentHumidity - humMean) > samuelsonDerivedThreshold * humStdDev) {
                result.badHumidity = true;
            }
        }
        //Write into buffer
        buffer[writeIndex] = reading;
        writeIndex = (writeIndex + 1) & capMask;
        writeCount++;
        return result;
        
        //GetFullBuffer checks if new multiple of 16 exists. If new multiple of 16 exists, buffer has been completely replaced, explose for curler
    }
        bool GetFullBuffer(Sensor::RefinedReadings bufferAccess[16]) {
            if (writeCount > 0 && (writeCount % 16) == 0) {
                for (int i = 0; i < bufferCap; i++) {
                bufferAccess[i] = buffer[i];
            }
            return true;
        }
        return false;
    }
    private:
    //Buffer capacity of 16, power of 2, for bitwise operations.
    static constexpr int bufferCap = 16;
    static constexpr int capMask = bufferCap - 1;
    static constexpr double samuelsonDerivedThreshold = 2.5;


    Sensor::RefinedReadings buffer[bufferCap];
    //Add reader later for sending batched readings
    int writeIndex = 0;
    unsigned long long writeCount = 0;
    
    int currentBufferSize() const {
        return static_cast<int>(std::min<unsigned long long>(writeCount, bufferCap));
    }
    //Z-score setup, current buffer size makes Z-score analysis less reliable, can be expanded for new implementations later
    //Z score is z = x - mean divded by standard deviation
    //Standard deviation
    double meanTemperature() const{
        int n = currentBufferSize();
        double total = 0.0;
        for (int i = 0; i < n; ++i) {
            total += (buffer[i].Temperature / 10.0);
        }
        return total / n; 
    }
        double stdDevTemperature(double mean) const {
        int n = currentBufferSize();
        double sumSquares = 0.0;
        for (int i = 0; i < n; ++i) {
            double difference = (buffer[i].Temperature / 10.0) - mean;
            sumSquares += difference * difference;
        }
            return std::sqrt(sumSquares / n);
    }
        double meanHumidity() const {
        int n = currentBufferSize();
        double total = 0.0;
        for (int i = 0; i < n; ++i) {
                total += (buffer[i].Humidity / 10.0);
        }
        return total / n;
    }
        double stdDevHumidity(double mean) {
        int n = currentBufferSize();
        double sumSquares = 0.0;
        for (int i = 0; i < n; ++i) {
            double difference = (buffer[i].Humidity / 10.0) - mean;
            sumSquares += difference * difference;
    }
        return std::sqrt(sumSquares / n);
    }
};