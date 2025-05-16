
cmake_minimum_required(VERSION 3.20.0)
set(USB_SOURCES ${CMAKE_CURRENT_LIST_DIR}/usbdevice.cpp)
set(USB_HEADER ${CMAKE_CURRENT_LIST_DIR}/usbdevice.h)


add_library(MYUSBDEVICE INTERFACE)
target_sources(MYUSBDEVICE INTERFACE ${USB_SOURCES} ${USB_HEADER})
target_include_directories(MYUSBDEVICE INTERFACE ${CMAKE_CURRENT_LIST_DIR} )