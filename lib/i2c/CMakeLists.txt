zephyr_library()
zephyr_include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
zephyr_library_sources(src/i2ccontroller.cpp src/24lc512.cpp)