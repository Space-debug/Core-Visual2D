cmake_minimum_required(VERSION 3.18)
project(visual2d_infer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/Detection2D.msg"
  "msg/Detection2DArray.msg"
  "msg/SegmentInstance.msg"
  "msg/SegmentResult.msg"
  DEPENDENCIES std_msgs
)

get_filename_component(CORE_VISUAL2D_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(CORE_VISUAL2D_LIB_DIR "${CORE_VISUAL2D_ROOT}/_LIBRARY/lib")
set(CORE_VISUAL2D_INCLUDE_DIR "${CORE_VISUAL2D_ROOT}/_LIBRARY/include")

if(NOT EXISTS "${CORE_VISUAL2D_LIB_DIR}/libVisual2DInfer.so")
  message(FATAL_ERROR
    "Core-Visual2D library not found at ${CORE_VISUAL2D_LIB_DIR}/libVisual2DInfer.so\n"
    "Please build the core library first: cd ${CORE_VISUAL2D_ROOT} && ./build.sh")
endif()

add_executable(visual2d_node src/visual2d_node.cpp)

target_include_directories(visual2d_node PRIVATE
  ${CORE_VISUAL2D_ROOT}/include
  ${CORE_VISUAL2D_INCLUDE_DIR}
  ${CORE_VISUAL2D_INCLUDE_DIR}/Visual2DInfer
)

target_link_directories(visual2d_node PRIVATE
  ${CORE_VISUAL2D_LIB_DIR}
)

target_link_libraries(visual2d_node
  Visual2DInfer
  trtyolo
  custom_plugins
  ${OpenCV_LIBS}
)

ament_target_dependencies(visual2d_node
  rclcpp
  sensor_msgs
  std_msgs
  cv_bridge
)

rosidl_get_typesupport_target(cpp_typesupport_target
  ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(visual2d_node "${cpp_typesupport_target}")

set_target_properties(visual2d_node PROPERTIES
  BUILD_RPATH "${CORE_VISUAL2D_LIB_DIR}"
  INSTALL_RPATH "${CORE_VISUAL2D_LIB_DIR};$ORIGIN"
  BUILD_RPATH_USE_ORIGIN ON
  INSTALL_RPATH_USE_LINK_PATH ON
)

install(TARGETS visual2d_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch config
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
