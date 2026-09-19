# Standard ESP-IDF build system requirement
cmake_minimum_required(VERSION 3.16)

# Include main CMake macros for ESP-IDF projects
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Define the ESP32 firmware project name
project(ds3231_esp32c6_demo)