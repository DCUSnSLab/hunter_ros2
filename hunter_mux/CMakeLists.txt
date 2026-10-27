cmake_minimum_required(VERSION 3.8)
project(hunter_mux)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(teleop_rover_msgs REQUIRED)

add_library(mux_component SHARED src/mux_node.cpp)
ament_target_dependencies(mux_component rclcpp rclcpp_components geometry_msgs teleop_rover_msgs)
rclcpp_components_register_node(mux_component
  PLUGIN "hunter_mux::MuxNode"
  EXECUTABLE mux_node)

install(TARGETS mux_component
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)

install(DIRECTORY
  config
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_cmake_cppcheck REQUIRED)
  find_package(ament_cmake_lint_cmake REQUIRED)
  find_package(ament_cmake_xmllint REQUIRED)
  ament_cppcheck(src)
  ament_lint_cmake(CMakeLists.txt)
  ament_xmllint(package.xml)
endif()

ament_package()
