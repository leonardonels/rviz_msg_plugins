cmake_minimum_required(VERSION 3.8)
project(rviz_msg_plugins)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# RViz plugins use Qt widgets and signals/slots, so Qt's meta-object compiler
# (moc) must run automatically on headers that contain Q_OBJECT.
set(CMAKE_AUTOMOC ON)

# --- Dependencies -----------------------------------------------------------
find_package(ament_cmake REQUIRED)
find_package(pluginlib REQUIRED)
find_package(rviz_common REQUIRED)
find_package(rviz_rendering REQUIRED)
find_package(rviz_default_plugins REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(livox_ros_driver2 REQUIRED)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

# --- The plugin library -----------------------------------------------------
# RViz loads displays from a shared library at runtime. List every .cpp here.
# IMPORTANT: also list each Q_OBJECT *header* — AUTOMOC only auto-scans headers
# that sit next to a source of the same name. Ours live under include/, so they
# must be named explicitly or moc never runs and the class vtable is undefined
# (dlopen fails with "undefined symbol: ...LivoxCustomMsgDisplay...").
add_library(${PROJECT_NAME} SHARED
  src/livox_custom_msg_display.cpp
  include/rviz_msg_plugins/livox_custom_msg_display.hpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}
  Qt5::Widgets
)

ament_target_dependencies(${PROJECT_NAME}
  pluginlib
  rviz_common
  rviz_rendering
  rviz_default_plugins
  sensor_msgs
  livox_ros_driver2
)

# Some RViz headers expect this define for symbol visibility on the boundary.
target_compile_definitions(${PROJECT_NAME} PRIVATE "RVIZ_MSG_PLUGINS_BUILDING_LIBRARY")

# --- Plugin registration ----------------------------------------------------
# Tells pluginlib (via rviz_common) where to find the manifest at runtime.
pluginlib_export_plugin_description_file(rviz_common plugins_description.xml)

# --- Install ----------------------------------------------------------------
install(TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/
  DESTINATION include
)

install(FILES plugins_description.xml
  DESTINATION share/${PROJECT_NAME}
)

ament_export_include_directories(include)
ament_export_targets(export_${PROJECT_NAME})
ament_export_dependencies(
  pluginlib
  rviz_common
  rviz_rendering
  rviz_default_plugins
  livox_ros_driver2
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
