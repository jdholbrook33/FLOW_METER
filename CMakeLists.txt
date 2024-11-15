# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(FLOWMETER)


# In root CMakeLists.txt, add:
set(PARTITION_TABLE_CSV "partition.csv")

# Point to the new location
littlefs_create_partition_image(storage ${CMAKE_CURRENT_SOURCE_DIR}/littlefs FLASH_IN_PROJECT)
