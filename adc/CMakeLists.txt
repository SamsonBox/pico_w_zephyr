# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ADC)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/usb myusbedevice)

target_sources(app PRIVATE src/main.cpp)
target_link_libraries(app PRIVATE MYUSBDEVICE)
