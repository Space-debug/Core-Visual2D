# Test programs subdirectory
# This CMakeLists.txt is included from the parent CMakeLists.txt via add_subdirectory()

# Use parent project variables
set(PARENT_DIR "${CMAKE_SOURCE_DIR}")
set(OUTPUT_LIB_DIR "${PARENT_DIR}/_LIBRARY/lib")
set(OUTPUT_INCLUDE_DIR "${PARENT_DIR}/_LIBRARY/include")

# ================================================================================
# 1. Find Dependencies
# ================================================================================
# Find OpenCV
find_package(OpenCV REQUIRED)
if(OpenCV_FOUND)
    message(STATUS "OpenCV found: ${OpenCV_VERSION}")
    message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
    message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
else()
    message(FATAL_ERROR "OpenCV not found. Please install OpenCV.")
endif()

# Find OpenMP for parallel processing in convert.cpp
find_package(OpenMP QUIET)
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found")
else()
    message(WARNING "OpenMP not found. Performance may be reduced.")
endif()

# ================================================================================
# 2. Create convert library (shared by test programs)
# ================================================================================
add_library(test_convert STATIC
    convert.cpp
)

target_include_directories(test_convert PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(test_convert PUBLIC
    ${OpenCV_LIBS}
)

# Enable OpenMP for convert.cpp if available
if(OpenMP_CXX_FOUND)
    target_link_libraries(test_convert PUBLIC OpenMP::OpenMP_CXX)
    target_compile_options(test_convert PRIVATE ${OpenMP_CXX_FLAGS})
endif()

# ================================================================================
# 3. Test program: detect
# ================================================================================
add_executable(test_detect
    detect.cpp
)

target_include_directories(test_detect PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PARENT_DIR}/include
    ${OUTPUT_INCLUDE_DIR}
    ${OUTPUT_INCLUDE_DIR}/Visual2DInfer
)

target_link_directories(test_detect PRIVATE
    ${OUTPUT_LIB_DIR}
)

target_link_libraries(test_detect PRIVATE
    Visual2DInfer
    test_convert
    ${OpenCV_LIBS}
)

# Link subproject libraries explicitly (needed for operator<< symbols)
target_link_libraries(test_detect PRIVATE
    trtyolo
    custom_plugins
)

# Set RPATH to find Visual2DInfer library at runtime
set_target_properties(test_detect PROPERTIES
    BUILD_RPATH "${OUTPUT_LIB_DIR}"
    BUILD_RPATH_USE_ORIGIN ON
)

# ================================================================================
# 4. Test program: segment
# ================================================================================
add_executable(test_segment
    segment.cpp
)

target_include_directories(test_segment PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PARENT_DIR}/include
    ${OUTPUT_INCLUDE_DIR}
    ${OUTPUT_INCLUDE_DIR}/Visual2DInfer
)

target_link_directories(test_segment PRIVATE
    ${OUTPUT_LIB_DIR}
)

target_link_libraries(test_segment PRIVATE
    Visual2DInfer
    test_convert
    ${OpenCV_LIBS}
)

# Link subproject libraries explicitly (needed for operator<< symbols)
target_link_libraries(test_segment PRIVATE
    trtyolo
    custom_plugins
)

# Set RPATH to find Visual2DInfer library at runtime
set_target_properties(test_segment PROPERTIES
    BUILD_RPATH "${OUTPUT_LIB_DIR}"
    BUILD_RPATH_USE_ORIGIN ON
)

# ================================================================================
# 5. Print configuration summary
# ================================================================================
message(STATUS "Test programs configured:")
message(STATUS "  - test_detect")
message(STATUS "  - test_segment")
message(STATUS "  OpenCV version: ${OpenCV_VERSION}")
message(STATUS "  OpenMP: ${OpenMP_CXX_FOUND}")

