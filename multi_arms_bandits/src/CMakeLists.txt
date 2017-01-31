cmake_minimum_required(VERSION 2.6)

set(target_name "multi_arms_bandits")
project(${target_name})

find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB src *.cpp)
file(GLOB hdr *.h*)

add_executable(${target_name} ${src} ${hdr})
target_link_libraries(${target_name} ${OpenCV_LIBS})
