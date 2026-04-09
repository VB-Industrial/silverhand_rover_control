cmake_minimum_required(VERSION 3.16)
project(silverhand_rover_control)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(ament_cmake REQUIRED)
find_package(hardware_interface REQUIRED)
find_package(libcxxcanard REQUIRED)
find_package(pluginlib REQUIRED)
find_package(PkgConfig QUIET)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_srvs REQUIRED)

find_library(HIDAPI_HIDRAW_LIB hidapi-hidraw)
set(SDL2_LIB_TARGET "")
if(PKG_CONFIG_FOUND)
  pkg_check_modules(SDL2 QUIET IMPORTED_TARGET sdl2)
  if(TARGET PkgConfig::SDL2)
    set(SDL2_LIB_TARGET PkgConfig::SDL2)
    message(STATUS "Found SDL2 via pkg-config for HID fallback")
  endif()
endif()

if(HIDAPI_HIDRAW_LIB)
  message(STATUS "Found hidapi library: ${HIDAPI_HIDRAW_LIB}")
elseif(SDL2_LIB_TARGET)
  message(STATUS "hidapi-hidraw not found, using SDL2 HID fallback")
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

target_compile_definitions(${PROJECT_NAME}_cyphal PRIVATE SILVERHAND_ROVER_CONTROL_BUILDING_DLL)
ament_target_dependencies(${PROJECT_NAME}_cyphal libcxxcanard rclcpp)

add_library(${PROJECT_NAME} SHARED src/silverhand_rover_hardware.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_cyphal)
if(HIDAPI_HIDRAW_LIB)
  target_link_libraries(${PROJECT_NAME} ${HIDAPI_HIDRAW_LIB})
elseif(SDL2_LIB_TARGET)
  target_link_libraries(${PROJECT_NAME} ${SDL2_LIB_TARGET})
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE SILVERHAND_ROVER_CONTROL_BUILDING_DLL)

ament_target_dependencies(
  ${PROJECT_NAME}
  hardware_interface
  libcxxcanard
  pluginlib
  rclcpp
  rclcpp_lifecycle
)

add_executable(power_board_node src/power_board_node.cpp)
target_link_libraries(power_board_node ${PROJECT_NAME}_cyphal)
ament_target_dependencies(
  power_board_node
  libcxxcanard
  rclcpp
  sensor_msgs
  std_srvs
)

add_executable(headlights_node src/headlights_node.cpp)
target_link_libraries(headlights_node ${PROJECT_NAME}_cyphal)
ament_target_dependencies(
  headlights_node
  libcxxcanard
  rclcpp
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
  TARGETS ${PROJECT_NAME}_cyphal ${PROJECT_NAME} power_board_node headlights_node
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

ament_export_dependencies(
  hardware_interface
  libcxxcanard
  pluginlib
  rclcpp
  rclcpp_lifecycle
  sensor_msgs
  std_srvs
)
ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME})

ament_package()
