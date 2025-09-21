zephyr_library()
zephyr_include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
zephyr_library_sources(src/pushover.cpp)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
foreach(resource pushover-net.der)
  message(STATUS "generating file: " ${gen_dir}/${resource}.inc)
  generate_inc_file_for_target(
    app
    include/static_includes/${resource}
    ${gen_dir}/${resource}.inc
  )

endforeach()