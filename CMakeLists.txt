cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(FLOWMETER)

# Point to the new location
littlefs_create_partition_image(storage littlefs FLASH_IN_PROJECT)

project(FLOWMETER)