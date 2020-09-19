# Modfied from luajit.cmake
# Added LUA_ADD_EXECUTABLE Ryan Phillips <ryan at trolocsis.com>
# This CMakeLists.txt has been first taken from LuaDist
# Copyright (C) 2007-2011 LuaDist.
# Created by Peter Drahoš
# Redistribution and use of this file is allowed according to the terms of the
# MIT license.
# Debugged and (now seriously) modIFied by Ronan Collobert, for Torch7

PROJECT(lua C)

IF(NOT LUA_DIR)
  MESSAGE(FATAL_ERROR "Must set LUA_DIR to build lua with CMake")
ENDIF()

FILE(COPY ${CMAKE_CURRENT_LIST_DIR}/luauser.h DESTINATION ${CMAKE_BINARY_DIR})

SET(CMAKE_REQUIRED_INCLUDES
  ${LUA_DIR}
  ${CMAKE_BINARY_DIR})

# Ugly warnings
IF(MSVC)
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
ENDIF()

# Various includes
INCLUDE(CheckLibraryExists)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckCSourceCompiles)
INCLUDE(CheckTypeSize)

IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
  ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE)
ENDIF()

IF(NOT WIN32)
  FIND_LIBRARY(DL_LIBRARY "dl")
  IF(DL_LIBRARY)
    SET(CMAKE_REQUIRED_LIBRARIES ${DL_LIBRARY})
    LIST(APPEND LIBS ${DL_LIBRARY})
  ENDIF()
  CHECK_FUNCTION_EXISTS(dlopen LUA_USE_DLOPEN)
  IF(NOT LUA_USE_DLOPEN)
    MESSAGE(FATAL_ERROR "Cannot compile a useful lua.
Function dlopen() seems not to be supported on your platform.
Apparently you are not on a Windows platform as well.
So lua has no way to deal with shared libraries!")
  ENDIF()
ENDIF()

CHECK_LIBRARY_EXISTS(m sin "" LUA_USE_LIBM)
IF( LUA_USE_LIBM )
  LIST( APPEND LIBS m )
ENDIF()

## SOURCES
SET(SRC_LUALIB
  ${LUA_DIR}/lbaselib.c
  ${LUA_DIR}/lcorolib.c
  ${LUA_DIR}/ldblib.c
  ${LUA_DIR}/liolib.c
  ${LUA_DIR}/lmathlib.c
  ${LUA_DIR}/loadlib.c
  ${LUA_DIR}/loslib.c
  ${LUA_DIR}/lstrlib.c
  ${LUA_DIR}/ltablib.c
  ${LUA_DIR}/lutf8lib.c
)

SET(SRC_LUACORE
  ${LUA_DIR}/lauxlib.c
  ${LUA_DIR}/lapi.c
  ${LUA_DIR}/lcode.c
  ${LUA_DIR}/lctype.c
  ${LUA_DIR}/ldebug.c
  ${LUA_DIR}/ldo.c
  ${LUA_DIR}/ldump.c
  ${LUA_DIR}/lfunc.c
  ${LUA_DIR}/lgc.c
  ${LUA_DIR}/linit.c
  ${LUA_DIR}/llex.c
  ${LUA_DIR}/lmem.c
  ${LUA_DIR}/lobject.c
  ${LUA_DIR}/lopcodes.c
  ${LUA_DIR}/lparser.c
  ${LUA_DIR}/lstate.c
  ${LUA_DIR}/lstring.c
  ${LUA_DIR}/ltable.c
  ${LUA_DIR}/ltm.c
  ${LUA_DIR}/lundump.c
  ${LUA_DIR}/lvm.c
  ${LUA_DIR}/lzio.c
  ${SRC_LUALIB}
)

## GENERATE
IF(WITH_SHARED_LUA)
  IF(IOS OR ANDROID)
    SET(LIBTYPE STATIC)
  ELSE()
    SET(LIBTYPE SHARED)
  ENDIF()
ELSE()
  SET(LIBTYPE STATIC)
ENDIF()
ADD_LIBRARY(lualib ${LIBTYPE} ${SRC_LUACORE})
SET(LUA_COMPILE_DEFINITIONS)
IF(ANDROID OR IOS)
  LIST(APPEND LUA_COMPILE_DEFINITIONS LUA_USER_H="luauser.h")
  INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
ENDIF()
IF(NOT WIN32)
  LIST(APPEND LUA_COMPILE_DEFINITIONS "LUA_USE_POSIX")
ENDIF()

SET_TARGET_PROPERTIES(lualib PROPERTIES
  PREFIX "lib"
  IMPORT_PREFIX "lib"
  COMPILE_DEFINITIONS "${LUA_COMPILE_DEFINITIONS}"
)
IF(LUA_COMPILE_FLAGS)
  SET_TARGET_PROPERTIES(lualib PROPERTIES
    COMPILE_FLAGS ${LUA_COMPILE_FLAGS})
ENDIF()

TARGET_LINK_LIBRARIES(lualib ${LIBS})
SET_TARGET_PROPERTIES(lualib PROPERTIES OUTPUT_NAME "lua53")
LIST(APPEND LIB_LIST lualib)

ADD_EXECUTABLE(lua ${LUA_DIR}/lua.c)
IF(WIN32)
  TARGET_LINK_LIBRARIES(lua lualib)
ELSE()
  TARGET_LINK_LIBRARIES(lua lualib ${LIBS})
  SET_TARGET_PROPERTIES(lua PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    ENABLE_EXPORTS ON)
ENDIF()

SET(lua_headers
  ${LUA_DIR}/lauxlib.h
  ${LUA_DIR}/lua.h
  ${LUA_DIR}/luaconf.h
  ${LUA_DIR}/lualib.h)
INSTALL(FILES ${lua_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/lua)
INSTALL(TARGETS lualib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

INSTALL(TARGETS lua DESTINATION "${CMAKE_INSTALL_BINDIR}")
