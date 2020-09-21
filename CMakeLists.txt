cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(USE_LUAJIT ON CACHE BOOL "Prefer luajit over lua")

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

file(GLOB MOD_SOURCES "src/mods/*.c")

add_executable(${PROJECT_NAME} WIN32
  src/dwm-win32.c
  src/win32_utf8.c
  ${MOD_SOURCES})

if (USE_LUAJIT)
  target_link_libraries(${PROJECT_NAME} libluajit)
else()
  target_link_libraries(${PROJECT_NAME} liblua)
endif()

target_link_libraries(${PROJECT_NAME} luacompat53)

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

set(luacompat53_SOURCE_DIR "${PROJECT_SOURCE_DIR}/extern/lua-compat-5.3")
FILE(GLOB luacompat53_sources ${luacompat53_SOURCE_DIR}/*.c)
add_library(luacompat53 STATIC ${luacompat53_sources})
target_include_directories(luacompat53
  PUBLIC
  $<BUILD_INTERFACE:${luacompat53_SOURCE_DIR}>
)
if (USE_LUAJIT)
  target_link_libraries(luacompat53 libluajit)
else()
  target_link_libraries(luacompat53 liblua)
endif()

if (USE_LUAJIT)
  set(LUAJIT_ENABLE_LUA52COMPAT ON)
  set(LUAJIT_BUILD_ALAMG ON)
  set(LUAJIT_DIR ${CMAKE_CURRENT_LIST_DIR}/extern/luajit)
  include(${CMAKE_CURRENT_LIST_DIR}/cmake/luajit/luajit.cmake)
  target_include_directories(libluajit
    PUBLIC
    $<BUILD_INTERFACE:${LUAJIT_DIR}/src>
  )
else()
  set(lua_SOURCE_DIR "${PROJECT_SOURCE_DIR}/extern/lua/src")
  FILE(GLOB lua_sources ${lua_SOURCE_DIR}/*.c)
  list(REMOVE_ITEM lua_sources "${lua_SOURCE_DIR}/lua.c" "${lua_SOURCE_DIR}/luac.c")
  add_library(liblua STATIC ${lua_sources})
  target_include_directories(liblua
    PUBLIC
    $<BUILD_INTERFACE:${lua_SOURCE_DIR}>
  )
endif()
