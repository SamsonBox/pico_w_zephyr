zephyr_library()
zephyr_include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
zephyr_library_sources(src/status_led.cpp)