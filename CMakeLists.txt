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

# ================================================================================
# 1. Output Folder: _LIBRARY
# ================================================================================
set(OUTPUT_FOLDER "_LIBRARY" CACHE PATH "Directory to output library and headers")
set(OUTPUT_BASE "${CMAKE_CURRENT_LIST_DIR}/${OUTPUT_FOLDER}")
set(OUTPUT_INCLUDE_DIR "${OUTPUT_BASE}/include")
set(OUTPUT_PROJECT_INCLUDE_DIR "${OUTPUT_INCLUDE_DIR}/${PROJECT_NAME}")
set(OUTPUT_LIB_DIR "${OUTPUT_BASE}/lib")

# ================================================================================
# 2. CUDA Configuration
# ================================================================================
set(CMAKE_CUDA_ARCHITECTURES "87")
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -O3")

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
    COMMENT "Copying headers, library and 3rdparty to ${OUTPUT_FOLDER}/"
)
