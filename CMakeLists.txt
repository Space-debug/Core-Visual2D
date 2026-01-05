cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
project(Visual2DInfer VERSION 1.0.0 LANGUAGES CXX CUDA)

# Include GNUInstallDirs to get standard installation directories
include(GNUInstallDirs)

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
find_package(CUDAToolkit REQUIRED)
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
# 2.1 TensorRT Path Configuration
# ================================================================================
set(TRT_PATH "/usr/src/tensorrt" CACHE PATH "Path to TensorRT installation (e.g., /usr/local/tensorrt)")
if(NOT TRT_PATH)
    message(FATAL_ERROR "TensorRT path must be specified with -DTRT_PATH=/path/to/tensorrt")
endif()

set(TRT_LIB_DIR "${TRT_PATH}/lib")
if(MSVC AND EXISTS "${TRT_LIB_DIR}/nvinfer_10.dll")
    set(TRT_LIBS nvinfer_10 nvinfer_plugin_10 nvonnxparser_10)
else()
    set(TRT_LIBS nvinfer nvinfer_plugin nvonnxparser)
endif()

# ================================================================================
# 2.2. Define the configure_cuda_trt_target function
# ================================================================================
function(configure_cuda_trt_target target)
    target_include_directories(${target} PRIVATE ${TRT_PATH}/include)
    target_link_directories(${target} PRIVATE ${TRT_LIB_DIR})
    target_link_libraries(${target} PRIVATE CUDA::cudart ${TRT_LIBS})
    set_target_properties(${target} PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
        CUDA_RESOLVE_DEVICE_SYMBOLS ON
    )
    target_compile_features(${target} PRIVATE cxx_std_17)
endfunction()

# ================================================================================
# 2.3. Define the set_target_compile_options function
# ================================================================================
function(set_target_compile_options target)
    target_compile_options(${target} PRIVATE
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
        $<$<CONFIG:RelWithDebInfo>:-O2 -g -DNDEBUG>
        $<$<CONFIG:MinSizeRel>:-Os -DNDEBUG>
    )
endfunction()


# ================================================================================
# 3. Add Subprojects
# ================================================================================
# 添加子项目，使 OUTPUT_LIB_DIR 在子项目中可用
add_subdirectory(modules/trtyolo)
add_subdirectory(modules/plugin)

# ================================================================================
# 3.1. Other dependencies (uncomment if needed)
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

# Apply TensorRT/CUDA configuration to the main target
configure_cuda_trt_target(${PROJECT_NAME})

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

# Create deploy directory mapping for trtyolo.hpp at compile time
set(DEPLOY_DIR "${PROJECT_SOURCE_DIR}/include/deploy")
file(MAKE_DIRECTORY ${DEPLOY_DIR})
# Create a symlink or copy trtyolo.hpp to deploy directory for compilation
configure_file(
    "${PROJECT_SOURCE_DIR}/modules/trtyolo/infer/trtyolo.hpp"
    "${DEPLOY_DIR}/trtyolo.hpp"
    COPYONLY
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${OUTPUT_INCLUDE_DIR}
)

# ================================================================================
# 6. Link Libraries
# ================================================================================
target_link_directories(${PROJECT_NAME} PRIVATE
    ${OUTPUT_LIB_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    trtyolo
    custom_plugins
)

# ================================================================================
# 7. Copy Headers and Library to _LIBRARY/ after build
# ================================================================================
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_PROJECT_INCLUDE_DIR} ${OUTPUT_INCLUDE_DIR} ${OUTPUT_LIB_DIR} ${OUTPUT_INCLUDE_DIR}/deploy
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/include/Visual2DInfer.h ${OUTPUT_PROJECT_INCLUDE_DIR}/Visual2DInfer.h
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/modules/trtyolo/infer/trtyolo.hpp ${OUTPUT_INCLUDE_DIR}/deploy/trtyolo.hpp
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}> ${OUTPUT_LIB_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:trtyolo> ${OUTPUT_LIB_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:custom_plugins> ${OUTPUT_LIB_DIR}/
    COMMENT "Copying headers and library to ${OUTPUT_FOLDER}/ (${CMAKE_BUILD_TYPE})"
)

# ================================================================================
# 8. Test Programs (optional)
# ================================================================================
option(BUILD_TEST_PROGRAMS "Build test programs (detect.cpp and segment.cpp)" OFF)

if(BUILD_TEST_PROGRAMS)
    add_subdirectory(test)
endif()
