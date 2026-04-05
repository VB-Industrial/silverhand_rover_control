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

target_link_libraries(${PROJECT_NAME} libcxxcanard)
target_compile_definitions(${PROJECT_NAME} PRIVATE SILVERHAND_ROVER_CONTROL_BUILDING_DLL)

ament_target_dependencies(
  ${PROJECT_NAME}
  hardware_interface
  pluginlib
  rclcpp
  rclcpp_lifecycle
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
  TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

ament_export_dependencies(
  hardware_interface
  pluginlib
  rclcpp
  rclcpp_lifecycle
)
ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME})

ament_package()
