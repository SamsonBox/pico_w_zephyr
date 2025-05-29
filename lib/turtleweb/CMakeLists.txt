zephyr_library()
zephyr_include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
zephyr_library_sources(src/turtle_http_server.cpp)

zephyr_linker_sources(SECTIONS sections-rom.ld)
zephyr_linker_section_ifdef(CONFIG_NET_SAMPLE_HTTPS_SERVICE NAME
				http_resource_desc_test_https_service
				KVMA RAM_REGION GROUP RODATA_REGION
				SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
zephyr_linker_section_ifdef(CONFIG_NET_SAMPLE_HTTP_SERVICE NAME
				http_resource_desc_test_http_service
				KVMA RAM_REGION GROUP RODATA_REGION
				SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
foreach(web_resource
  index.html
  main.js
    )
  message(STATUS "generating file: " ${gen_dir}/${web_resource}.gz.inc)
  generate_inc_file_for_target(
    app
    static_web_resources/${web_resource}
    ${gen_dir}/${web_resource}.gz.inc
    --gzip
  )

endforeach()