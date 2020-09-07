cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

set(CMAKE_C_COMPILER clang)
# set (CMAKE_C_FLAGS "-m32")
set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type.
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "CMAKE_BUILD_TYPE not specified, default is 'Debug'")
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build" FORCE)
else()
  message(STATUS "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
endif()
if(CMAKE_BUILD_TYPE MATCHES Debug)
  set(DEBUG 1)
else()
  set(DEBUG 0)
endif()

project(dwm-win32
  VERSION 0.0.50
  DESCRIPTION "Dynamic Window Manager for Win32"
  LANGUAGES C)

add_executable(${PROJECT_NAME} WIN32
  src/dwm-win32.c)

configure_file(
  "${PROJECT_SOURCE_DIR}/config.def.h"
  "${PROJECT_SOURCE_DIR}/src/config.h"
)

if (NOT MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    # -Werror
    -Wpedantic
  )
endif()
