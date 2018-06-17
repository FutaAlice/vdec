project (vdec)

if (CMAKE_CL_64)
    set (PLATFORM x64)
else (CMAKE_CL_64)
    set (PLATFORM x86)
endif (CMAKE_CL_64)

include_directories (${VDEC_REPO_DIRECTORY}/include/vdec)
include_directories (${PREBUILT_DIRECTORY}/ffmpeg/include)

if (WIN32)
    set (FFMPEG_LIBRARY_PREFIX "lib")
    link_directories (${PREBUILT_DIRECTORY}/ffmpeg/lib/${PLATFORM})
elseif (UNIX)
    set (FFMPEG_LIBRARY_PREFIX "")
endif()

# Header and source files
file (GLOB HEADER_FILES ${VDEC_REPO_DIRECTORY}/include/vdec/*.h)
file (GLOB HEADER_FILES ./*.h)
aux_source_directory ("." SOURCE_FILES)

# Build as SHARED library
if (WIN32)
    add_definitions (-DVDEC_EXPORTS)
endif ()
add_library (${PROJECT_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES})

# Set include directory
target_include_directories (${PROJECT_NAME} INTERFACE ${VDEC_REPO_DIRECTORY}/include)

# Release link libraries
target_link_libraries (${PROJECT_NAME} PRIVATE optimized ${FFMPEG_LIBRARY_PREFIX}avutil)
target_link_libraries (${PROJECT_NAME} PRIVATE optimized ${FFMPEG_LIBRARY_PREFIX}avformat)
target_link_libraries (${PROJECT_NAME} PRIVATE optimized ${FFMPEG_LIBRARY_PREFIX}avcodec)
target_link_libraries (${PROJECT_NAME} PRIVATE optimized ${FFMPEG_LIBRARY_PREFIX}swresample)
target_link_libraries (${PROJECT_NAME} PRIVATE optimized ${FFMPEG_LIBRARY_PREFIX}swscale)

# Debug link libraries
target_link_libraries (${PROJECT_NAME} PRIVATE debug ${FFMPEG_LIBRARY_PREFIX}avutild)
target_link_libraries (${PROJECT_NAME} PRIVATE debug ${FFMPEG_LIBRARY_PREFIX}avformatd)
target_link_libraries (${PROJECT_NAME} PRIVATE debug ${FFMPEG_LIBRARY_PREFIX}avcodecd)
target_link_libraries (${PROJECT_NAME} PRIVATE debug ${FFMPEG_LIBRARY_PREFIX}swresampled)
target_link_libraries (${PROJECT_NAME} PRIVATE debug ${FFMPEG_LIBRARY_PREFIX}swscaled)
