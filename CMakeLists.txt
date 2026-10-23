cmake_minimum_required(VERSION 3.15)
project(cl_color LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "cl_color is Windows-only (uses the Win32 API)")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(cl_color cl_color.cpp)

install(TARGETS cl_color RUNTIME DESTINATION bin)
