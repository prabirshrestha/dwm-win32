cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

set(CMAKE_C_STANDARD 11)
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
  VERSION 0.2.0
  DESCRIPTION "Dynamic Window Manager for Win32"
  LANGUAGES C)

# set_target_properties(${PROJECT_NAME} PROPERTIES C_STANDARD 11)

add_executable(${PROJECT_NAME} WIN32
  src/dwm-win32.c)

target_link_libraries(${PROJECT_NAME} lua SDL2-static)

configure_file(
  "${PROJECT_SOURCE_DIR}/config.h.in"
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

set(lua_SOURCE_DIR "${PROJECT_SOURCE_DIR}/extern/lua/src")
FILE(GLOB lua_sources ${lua_SOURCE_DIR}/*.c)
list(REMOVE_ITEM lua_sources "${lua_SOURCE_DIR}/lua.c" "${lua_SOURCE_DIR}/luac.c")
add_library(lua STATIC ${lua_sources})
target_include_directories(lua
  PUBLIC
  $<BUILD_INTERFACE:${lua_SOURCE_DIR}>
)

set(sdl2_SOURCE_DIR "${PROJECT_SOURCE_DIR}/extern/sdl2")
add_subdirectory("${sdl2_SOURCE_DIR}")
target_include_directories(SDL2-static
  PUBLIC
  $<BUILD_INTERFACE:${sdl2_SOURCE_DIR}/include>
)
