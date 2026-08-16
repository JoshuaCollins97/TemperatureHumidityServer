//Header file for implementation of POST functionality
//Utilizing libcurl to make sensor readings available to webserver utilizing mongodb
#pragma once

#include <string>
#include <sstream>

//External Libraries: LibCURL
#include <curl/curl.h>

//Personal Includes
#include "Sensor.h"
#include "buffer.h"

class Poster {
    public:
        void CurlProcessRunner(RingBuffer& ringBuffer) {
            Sensor::RefinedReadings batch[16];

            if (ringBuffer.GetFullBuffer(batch)) {
                std::string jsonPayload = PayloadBuilder(batch);
                //curl it here
                CurlPoster(jsonPayload);
            }
        }
    private:
            std::string PayloadBuilder(const Sensor::RefinedReadings batch[16]) {
                std::string json = "{\n \"readings\": [\n";
                //Full batch
                for (int i=0; i < 16; i++) {
                    json += "   {\n";
                    json += "   \"Index\": " + std::to_string(i) + ",\n";
                    json += "   \"Temperature\": " + std::to_string(batch[i].Temperature) + ",\n";
                    json += "   \"Humidity\": " + std::to_string(batch[i].Humidity) + ",\n";
                    json += "   \"Timestamp\": " + std::to_string(batch[i].Timestamp) + "\n";
                    json += (i < 15) ? "    },\n" : "    }\n";
                }
                json += "  ]\n}";
                return json;

            }
            void CurlPoster(const std::string& payload) {
                CURL* curl = curl_easy_init();
                if (!curl) {
                    std::cerr << "Curler not Curling\n";
                    return;
                }

                std::string url = "http://192.168.0.100:3000/data/readings";

                struct curl_slist* headers = NULL;
                headers = curl_slist_append(headers, "Content-Type: application/json");

                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

                CURLcode res = curl_easy_perform(curl);
                if (res != CURLE_OK) {
                    std::cerr << "CURL Performance Failed: " << curl_easy_strerror(res) << "\n";
                }   else {
                    std::cout << "Stamped and Posted \n";
                }
                //Cleaners
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                }
            };
        