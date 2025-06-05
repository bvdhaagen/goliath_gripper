cmake_minimum_required(VERSION 3.16)
project(gripper_controller LANGUAGES CXX)

if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
  add_compile_options(-Wall -Wextra)
endif()

# set the same behavior for windows as it is on linux
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)



# find dependencies
set(HW_IF_INCLUDE_DEPENDS
  pluginlib
  rcpputils
  hardware_interface
)
set(REF_GEN_INCLUDE_DEPENDS
  kdl_parser
  rclcpp
  trajectory_msgs
)
set(CONTROLLER_INCLUDE_DEPENDS
  pluginlib
  rcpputils
  controller_interface
  realtime_tools
  trajectory_msgs
)

# find dependencies
find_package(backward_ros REQUIRED)
find_package(ament_cmake REQUIRED)
foreach(Dependency IN ITEMS ${HW_IF_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()
foreach(Dependency IN ITEMS ${REF_GEN_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()
foreach(Dependency IN ITEMS ${CONTROLLER_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()



add_library(
  gripper_controller
  SHARED
  hardware/gripper_hardware.cpp
)

target_compile_features(gripper_controller PUBLIC cxx_std_17)
target_include_directories(gripper_controller PUBLIC
$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/hardware/include>
$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/controller/include>
$<INSTALL_INTERFACE:include/gripper_controller>
)
ament_target_dependencies(
  gripper_controller PUBLIC
  ${HW_IF_INCLUDE_DEPENDS}
  ${CONTROLLER_INCLUDE_DEPENDS}
)

# Export hardware plugins
pluginlib_export_plugin_description_file(hardware_interface gripper_controller.xml)
# Export controller plugins
pluginlib_export_plugin_description_file(controller_interface gripper_controller.xml)

# INSTALL
install(
  DIRECTORY hardware/include/
  DESTINATION include/gripper_controller
)
install(
  DIRECTORY description/launch description/ros2_control description/urdf
  DESTINATION share/gripper_controller
)
install(
  DIRECTORY bringup/launch bringup/config
  DESTINATION share/gripper_controller
)

install(TARGETS gripper_controller
  EXPORT export_gripper_controller
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)


# After ament_target_dependencies(), add:
target_link_libraries(${PROJECT_NAME} PRIVATE
  Boost::thread  # Modern CMake style (preferred over ${Boost_LIBRARIES})
)



find_package(pluginlib REQUIRED)

# Add this near other find_package calls
find_package(Boost REQUIRED COMPONENTS thread)

if(BUILD_TESTING)
  find_package(ament_cmake_pytest REQUIRED)

  ament_add_pytest_test(example_7_urdf_xacro test/test_urdf_xacro.py)
  ament_add_pytest_test(view_example_7_launch test/test_view_robot_launch.py)
  ament_add_pytest_test(run_example_7_launch test/test_goliath_controller_launch.py)
endif()

## EXPORTS
ament_export_targets(export_gripper_controller HAS_LIBRARY_TARGET)
ament_export_dependencies(${HW_IF_INCLUDE_DEPENDS} ${REF_GEN_INCLUDE_DEPENDS} ${CONTROLLER_INCLUDE_DEPENDS})
ament_package()
