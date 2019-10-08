cmake_minimum_required(VERSION 3.10)
set(target_name "card_games")

project(${target_name})

find_package(OpenCV)
include_directories(${OpenCV_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB SRCS *.cpp)
file(GLOB HDRS *.h*)

add_executable(${target_name} ${SRCS} ${HDRS})
target_link_libraries(${target_name} opencv_core opencv_imgcodecs opencv_highgui opencv_imgproc)
set_target_properties(${target_name} PROPERTIES CXX_STANDARD 17)