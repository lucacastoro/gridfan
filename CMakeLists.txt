cmake_minimum_required(VERSION 3.8) # CMAKE_CXX_STANDARD = 17 requires CMake 3.8
project(gridfan)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(SYSTEMD_DIR /etc/systemd/system/)

find_library(LIBSENSORS sensors)

if(NOT LIBSENSORS)
  message(FATAL_ERROR "could not find libsensor")
endif()

add_subdirectory(libgridfan)
add_subdirectory(gridfan)
install(FILES gridfan.service DESTINATION ${SYSTEMD_DIR})
