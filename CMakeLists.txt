cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
project(Visual2DInfer VERSION 1.0.0 LANGUAGES CXX CUDA)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type to Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Validate build type
if(NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|Release|RelWithDebInfo|MinSizeRel)$")
    message(FATAL_ERROR "Invalid build type: ${CMAKE_BUILD_TYPE}. Must be one of: Debug, Release, RelWithDebInfo, MinSizeRel")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# ================================================================================
# 1. Output Folder: _LIBRARY
# ================================================================================
set(OUTPUT_FOLDER "_LIBRARY" CACHE PATH "Directory to output library and headers")
set(OUTPUT_BASE "${CMAKE_CURRENT_LIST_DIR}/${OUTPUT_FOLDER}")
set(OUTPUT_INCLUDE_DIR "${OUTPUT_BASE}/include")
set(OUTPUT_PROJECT_INCLUDE_DIR "${OUTPUT_INCLUDE_DIR}/${PROJECT_NAME}")
set(OUTPUT_LIB_DIR "${OUTPUT_BASE}/lib")

# ================================================================================
# 2. CUDA Configuration (build type specific)
# ================================================================================
set(CMAKE_CUDA_ARCHITECTURES "87")

# CUDA flags based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Debug: No optimization, include debug symbols
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -g -G -O0")
    set(CMAKE_CUDA_FLAGS_DEBUG "-g -G -O0")
else()
    # Release: Full optimization
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -O3")
    set(CMAKE_CUDA_FLAGS_RELEASE "-O3")
endif()

# ================================================================================
# 3. Other dependencies (uncomment if needed)
# ================================================================================
# find_package(OpenCV REQUIRED)
# include_directories(${OpenCV_INCLUDE_DIRS})

# ================================================================================
# 4. Source and Header Files
# ================================================================================
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.cxx"
    "${PROJECT_SOURCE_DIR}/src/*.cu"
)

file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/include/*.h"
    "${PROJECT_SOURCE_DIR}/include/*.hpp"
    "${PROJECT_SOURCE_DIR}/include/*.cuh"
)

# ================================================================================
# 5. Create Shared Library
# ================================================================================
add_library(${PROJECT_NAME} SHARED
    ${SRC_FILES}
    ${HEADER_FILES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_RESOLVE_DEVICE_SYMBOLS ON
    LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_LIB_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_LIB_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_LIB_DIR}
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
    BUILD_RPATH_USE_ORIGIN ON
    INSTALL_RPATH_USE_LINK_PATH ON
    POSITION_INDEPENDENT_CODE ON
)

# ================================================================================
# 2.5. Compiler flags based on build type
# ================================================================================
# C++ compiler flags for Debug
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
    $<$<CONFIG:RelWithDebInfo>:-O2 -g -DNDEBUG>
    $<$<CONFIG:MinSizeRel>:-Os -DNDEBUG>
)

# Add debug macro for Debug builds
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:DEBUG=1>
    $<$<CONFIG:Release>:NDEBUG=1>
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/3rdparty/include
)

# ================================================================================
# 6. Link Libraries
# ================================================================================
target_link_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/3rdparty/lib
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    trtyolo
    custom_plugins
)

# ================================================================================
# 7. Copy Headers, Library and 3rdparty to _LIBRARY/ after build
# ================================================================================
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_PROJECT_INCLUDE_DIR} ${OUTPUT_INCLUDE_DIR} ${OUTPUT_LIB_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/include ${OUTPUT_PROJECT_INCLUDE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}> ${OUTPUT_LIB_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/3rdparty/include ${OUTPUT_INCLUDE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/3rdparty/lib ${OUTPUT_LIB_DIR}
    COMMENT "Copying headers, library and 3rdparty to ${OUTPUT_FOLDER}/ (${CMAKE_BUILD_TYPE})"
)

# ================================================================================
# 8. Test Programs (optional)
# ================================================================================
option(BUILD_TEST_PROGRAMS "Build test programs (detect.cpp and segment.cpp)" OFF)

if(BUILD_TEST_PROGRAMS)
    add_subdirectory(test)
endif()
