#!/bin/bash
g++ publish_sensor.cpp I2CDevice.cpp ADXL345.cpp -o publish_sensor -lpaho-mqtt3c -std=c++2a
