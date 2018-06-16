project (vdec)

if (CMAKE_CL_64)
    set (PLATFORM x64)
else (CMAKE_CL_64)
    set (PLATFORM x86)
endif (CMAKE_CL_64)

include_directories (
    ${VDEC_REPO_DIRECTORY}/include/vdec
    ${PREBUILT_DIRECTORY}/ffmpeg/include
)
link_directories (${PREBUILT_DIRECTORY}/ffmpeg/lib/${PLATFORM})

# Header and source files
file (GLOB HEADER_FILES ${VDEC_REPO_DIRECTORY}/include/vdec/*.h)
file (GLOB HEADER_FILES ./*.h)
aux_source_directory ("." SOURCE_FILES)

# Build as SHARED library
add_definitions (-DVDEC_EXPORTS)
add_library (${PROJECT_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES})

# Set include directory
target_include_directories (${PROJECT_NAME} INTERFACE ${VDEC_REPO_DIRECTORY}/include)

# Release link libraries
target_link_libraries (${PROJECT_NAME} PRIVATE optimized libavutil)
target_link_libraries (${PROJECT_NAME} PRIVATE optimized libavformat)
target_link_libraries (${PROJECT_NAME} PRIVATE optimized libavcodec)
target_link_libraries (${PROJECT_NAME} PRIVATE optimized libswresample)
target_link_libraries (${PROJECT_NAME} PRIVATE optimized libswscale)

# Debug link libraries
target_link_libraries (${PROJECT_NAME} PRIVATE debug libavutild)
target_link_libraries (${PROJECT_NAME} PRIVATE debug libavformatd)
target_link_libraries (${PROJECT_NAME} PRIVATE debug libavcodecd)
target_link_libraries (${PROJECT_NAME} PRIVATE debug libswresampled)
target_link_libraries (${PROJECT_NAME} PRIVATE debug libswscaled)
