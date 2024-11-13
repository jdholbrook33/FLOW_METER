function(littlefs_create_partition_image)
    set(options FLASH_IN_PROJECT)
    set(multi DEPENDS)
    set(single PARTITION_NAME BASE_DIR)
    cmake_parse_arguments(arg "${options}" "${single}" "${multi}" "${ARGV}")

    if(NOT arg_BASE_DIR)
        message(STATUS "Creating LittleFS image from directory: ${arg_BASE_DIR}")
        set(arg_BASE_DIR "littlefs")
    endif()

    if(NOT arg_PARTITION_NAME)
        set(arg_PARTITION_NAME "storage")
    endif()

    set(image_file "${CMAKE_BINARY_DIR}/${arg_PARTITION_NAME}.bin")

    # Generate the image
    add_custom_command(
        OUTPUT ${image_file}
        COMMAND python ${IDF_PATH}/components/spiffs/spiffsgen.py
        ${littlefs_page_size} ${arg_BASE_DIR} ${image_file}
        DEPENDS ${arg_DEPENDS}
        VERBATIM
    )

    # Add the generated image to the flash target
    if(arg_FLASH_IN_PROJECT)
        esptool_py_flash_project_args(${arg_PARTITION_NAME} ${image_file})
    endif()

    # Add a target to generate the image
    add_custom_target(${arg_PARTITION_NAME}_bin DEPENDS ${image_file})
endfunction()