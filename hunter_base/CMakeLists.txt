cmake_minimum_required(VERSION 3.5)
project(hunter_base)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)

find_package(hunter_msgs REQUIRED)
find_package(ugv_sdk REQUIRED)
add_subdirectory(ascent)
# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)
include_directories(
  include
)


set(DEPENDENCIES
  "geometry_msgs"
  "nav_msgs"
  "rclcpp"
  "std_msgs"
  "tf2"
  "tf2_ros"
  "hunter_msgs"
  "sensor_msgs"
  "tf2_geometry_msgs"
)


add_library(hunter_base_component SHARED
    src/hunter_base_ros.cpp
    src/bicycle_model.cpp)
target_link_libraries(hunter_base_component ugv_sdk ascent)
ament_target_dependencies(hunter_base_component rclcpp rclcpp_components ${DEPENDENCIES})
rclcpp_components_register_node(hunter_base_component
  PLUGIN "westonrobot::HunterBaseRos"
  EXECUTABLE hunter_base_node)

add_executable(velocity_extractor_node
    src/velocity_extractor_node.cpp)
ament_target_dependencies(velocity_extractor_node rclcpp std_msgs hunter_msgs)

add_executable(hunter_state_parser_node
    src/hunter_state_parser_node.cpp)
ament_target_dependencies(hunter_state_parser_node rclcpp std_msgs hunter_msgs)

install(TARGETS hunter_base_component
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)

install(TARGETS velocity_extractor_node hunter_state_parser_node
  DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}/)

# install(DIRECTORY ascent
#   DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_cppcheck REQUIRED)
  find_package(ament_cmake_flake8 REQUIRED)
  find_package(ament_cmake_lint_cmake REQUIRED)
  find_package(ament_cmake_xmllint REQUIRED)
  ament_cppcheck(src include)
  ament_flake8(launch)
  ament_lint_cmake(CMakeLists.txt)
  ament_xmllint(package.xml)
endif()

ament_package()
