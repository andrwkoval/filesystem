cmake_minimum_required(VERSION 3.15)
project(filesystem C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64")
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

find_package(FUSE REQUIRED)

include_directories(./inc)
include_directories(${FUSE_INCLUDE_DIRS})

add_library(ffs_common SHARED src/ffs_common.c inc/ffs_common.h)

add_executable(mkfs.ffs src/ffs_mkfs.c inc/ffs_mkfs.h inc/ffs.h)
target_link_libraries(mkfs.ffs ${FUSE_LIBRARIES} ffs_common)
