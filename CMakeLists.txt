cmake_minimum_required(VERSION 3.16)
project(silverhand_rover_control)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(ament_cmake REQUIRED)
find_package(hardware_interface REQUIRED)
find_package(pluginlib REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_srvs REQUIRED)

set(
  LIBCXXCANARD_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libcxxcanard"
  CACHE PATH
  "Path to the libcxxcanard sources used by the future Cyphal transport layer"
)

if(NOT EXISTS "${LIBCXXCANARD_DIR}/CMakeLists.txt")
  message(FATAL_ERROR
    "libcxxcanard was not found at ${LIBCXXCANARD_DIR}. "
    "Run 'git submodule update --init --recursive' in the repository root."
  )
endif()

add_subdirectory("${LIBCXXCANARD_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/libcxxcanard")

find_library(HIDAPI_HIDRAW_LIB hidapi-hidraw)
if(HIDAPI_HIDRAW_LIB)
  message(STATUS "Found hidapi library: ${HIDAPI_HIDRAW_LIB}")
else()
  message(WARNING "hidapi-hidraw library was not found. USB IMU support will not link correctly.")
endif()

add_library(${PROJECT_NAME}_cyphal STATIC
  src/cyphal/runtime.cpp
  src/cyphal/motor_io.cpp
)

target_include_directories(${PROJECT_NAME}_cyphal PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_include_directories(${PROJECT_NAME}_cyphal PRIVATE
  "${LIBCXXCANARD_DIR}"
  "${LIBCXXCANARD_DIR}/libs"
  "${LIBCXXCANARD_DIR}/src"
)

target_link_libraries(${PROJECT_NAME}_cyphal libcxxcanard)
target_compile_definitions(${PROJECT_NAME}_cyphal PRIVATE SILVERHAND_ROVER_CONTROL_BUILDING_DLL)
ament_target_dependencies(${PROJECT_NAME}_cyphal rclcpp)

add_library(${PROJECT_NAME} SHARED src/silverhand_rover_hardware.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_include_directories(${PROJECT_NAME} PRIVATE
  "${LIBCXXCANARD_DIR}"
  "${LIBCXXCANARD_DIR}/libs"
  "${LIBCXXCANARD_DIR}/src"
)

target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_cyphal libcxxcanard)
if(HIDAPI_HIDRAW_LIB)
  target_link_libraries(${PROJECT_NAME} ${HIDAPI_HIDRAW_LIB})
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE SILVERHAND_ROVER_CONTROL_BUILDING_DLL)

ament_target_dependencies(
  ${PROJECT_NAME}
  hardware_interface
  pluginlib
  rclcpp
  rclcpp_lifecycle
)

add_executable(power_board_node src/power_board_node.cpp)
target_include_directories(power_board_node PRIVATE
  "${LIBCXXCANARD_DIR}"
  "${LIBCXXCANARD_DIR}/libs"
  "${LIBCXXCANARD_DIR}/src"
)
target_link_libraries(power_board_node ${PROJECT_NAME}_cyphal libcxxcanard)
ament_target_dependencies(
  power_board_node
  rclcpp
  sensor_msgs
  std_srvs
)

pluginlib_export_plugin_description_file(hardware_interface silverhand_rover_control.xml)

install(
  DIRECTORY config launch ros2_control urdf
  DESTINATION share/${PROJECT_NAME}
  PATTERN "__pycache__" EXCLUDE
  PATTERN "*.pyc" EXCLUDE
)

install(
  DIRECTORY include/
  DESTINATION include
)

install(
  FILES silverhand_rover_control.xml
  DESTINATION share/${PROJECT_NAME}
)

install(
  TARGETS ${PROJECT_NAME}_cyphal ${PROJECT_NAME} power_board_node
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

ament_export_dependencies(
  hardware_interface
  pluginlib
  rclcpp
  rclcpp_lifecycle
  sensor_msgs
  std_srvs
)
ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME})

ament_package()
