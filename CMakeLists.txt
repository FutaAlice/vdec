cmake_minimum_required (VERSION 2.8)

project (vdec)

if (CMAKE_CL_64)
    set (PLATFORM x64)
else (CMAKE_CL_64)
    set (PLATFORM x86)
endif (CMAKE_CL_64)

include_directories (
    prebuilt/ffmpeg/include
    include/vdec
)
link_directories (${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/ffmpeg/lib/${PLATFORM})

# Header and source files
file (GLOB HEADER_FILES "include/vdec/*.h")
file (GLOB HEADER_FILES "src/vdec/*.h")
aux_source_directory ("src/vdec" SOURCE_FILES)

# Build as SHARED library
add_library (${PROJECT_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES})

# Release link libraries
target_link_libraries (${PROJECT_NAME} optimized libavutil)
target_link_libraries (${PROJECT_NAME} optimized libavformat)
target_link_libraries (${PROJECT_NAME} optimized libavcodec)
target_link_libraries (${PROJECT_NAME} optimized libswresample)
target_link_libraries (${PROJECT_NAME} optimized libswscale)

# Debug link libraries
target_link_libraries (${PROJECT_NAME} debug libavutild)
target_link_libraries (${PROJECT_NAME} debug libavformatd)
target_link_libraries (${PROJECT_NAME} debug libavcodecd)
target_link_libraries (${PROJECT_NAME} debug libswresampled)
target_link_libraries (${PROJECT_NAME} debug libswscaled)
