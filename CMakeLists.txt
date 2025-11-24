# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(scheduler_feeder)

# ---- Header-only interfaces (ports) ----
add_subdirectory(interface)  # defines target: ports

# ---- Your libs (split by layer) ----
add_subdirectory(src/application)
add_subdirectory(src/devices/rtc)
add_subdirectory(src/devices/motor)
add_subdirectory(src/devices/watchdog)
add_subdirectory(src/devices/led)
add_subdirectory(src/devices/level_sensor)
add_subdirectory(src/network)
add_subdirectory(src/network/wifi)
add_subdirectory(src/network/mqtt)
add_subdirectory(src/network/json)
add_subdirectory(src/storage)




FILE(GLOB app_sources src/*.cpp)

target_sources(app PRIVATE ${app_sources})