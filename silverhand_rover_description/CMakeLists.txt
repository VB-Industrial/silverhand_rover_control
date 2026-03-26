cmake_minimum_required(VERSION 3.16)
project(silverhand_rover_description)

find_package(ament_cmake REQUIRED)

install(
  DIRECTORY config ros2_control urdf
  DESTINATION share/${PROJECT_NAME}
)

ament_package()

