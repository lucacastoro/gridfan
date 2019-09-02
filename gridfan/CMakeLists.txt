cmake_minimum_required(VERSION 3.1.3)
project(gridfan)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON) 
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(../libgridfan)

add_executable(${PROJECT_NAME} main.cpp temperature.cpp)
target_link_libraries(${PROJECT_NAME} ${LIBSENSORS} lib${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
