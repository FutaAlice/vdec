cmake_minimum_required (VERSION 2.8)

project(VDEC_PROJECT)

set (PREBUILT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt)
set (VDEC_REPO_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory (src/vdec)
add_subdirectory (test)