cmake_minimum_required(VERSION 2.8.3)
project(flight_strategy)

find_package(catkin REQUIRED COMPONENTS
  roscpp
  rospy
  std_msgs
  message_generation
  geometry_msgs
)

add_message_files(
  FILES
  ctrl.msg
)

generate_messages(
  DEPENDENCIES
  std_msgs
  geometry_msgs
)

catkin_package(
#  INCLUDE_DIRS include
#  LIBRARIES image_process
#  CATKIN_DEPENDS roscpp rospy std_msgs
#  DEPENDS system_lib
  CATKIN_DEPENDS message_runtime
)

include_directories(
  ${catkin_INCLUDE_DIRS}
  include
)

add_executable(strategy src/strategy.cpp)
target_link_libraries(strategy
  ${catkin_LIBRARIES}
)

add_executable(pos_ctrl src/pos_ctrl.cpp)
target_link_libraries(pos_ctrl
  ${catkin_LIBRARIES}
)
