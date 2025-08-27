# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(scheduler_feeder)


FILE(GLOB app_sources src/*.cpp)

target_sources(app PRIVATE ${app_sources})
add_subdirectory(src/devices/rtc)
add_subdirectory(src/devices/motor)
add_subdirectory(src/network/http/client)
add_subdirectory(src/network/wifi)
add_subdirectory(src/storage)
add_subdirectory(src/application)
