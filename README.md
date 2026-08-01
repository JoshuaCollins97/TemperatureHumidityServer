# TemperatureHumidityServer
https://cdn-learn.adafruit.com/assets/assets/000/123/394/original/Data_Sheet_AHT20.pdf
## Architectural Choices
The project currently uses a header only design. Functions and classes are both declared and defined within headers. The main file is
main.cpp, which acts as the project runner, importing all headers.

Current and planned headers are GPIOInterface.h, StateMachine.h, Sensor.h, and Serial.h

###GPIOInterface.h
GPIOInterface.h is currently the most extensive file, and should likely be split in the future. It contains the logic for the bit-banged
lcd, the buttons, and the LEDs. 


STANDARDS
Pascal Case - 
Functions
Class Names

camelCase - 
variable names
class instances

Capitalized snake case - 
low level hardware registers
Ex. SENS_TRIGGER


Sensor data sheet and information:
https://cdn-learn.adafruit.com/assets/assets/000/123/394/original/Data_Sheet_AHT20.pdf

LCD Datasheet and information
https://cdn.sparkfun.com/assets/9/5/f/7/b/HD44780.pdf