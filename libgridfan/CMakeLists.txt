cmake_minimum_required(VERSION 3.8) # CMAKE_CXX_STANDARD = 17 requires CMake 3.8
project(libgridfan)
set(CMAKE_CXX_STANDARD 14) 
set(CMAKE_CXX_STANDARD_REQUIRED ON) 
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(${PROJECT_NAME} SHARED libgridfan.cpp serial.c)
target_link_libraries(${PROJECT_NAME} ${LIBSENSORS})

