cmake_minimum_required(VERSION 3.8) # CMAKE_CXX_STANDARD = 17 requires CMake 3.8
project(gridfan)
set(SYSTEMD_DIR /etc/systemd/system/)
set(CONFIG_DIR /etc/gridfan/)

find_library(LIBSENSORS sensors)

if(NOT LIBSENSORS)
  message(FATAL_ERROR "could not find libsensor")
endif()

add_subdirectory(libgridfan)
add_subdirectory(gridfan)
add_subdirectory(yaml-cpp)
add_subdirectory(gflags)
install(FILES gridfan.service DESTINATION ${SYSTEMD_DIR})
install(FILES config.yml DESTINATION ${CONFIG_DIR})
