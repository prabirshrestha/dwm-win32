# dwm-win32 version
VERSION = alpha2

CPPFLAGS = -DVERSION=\"${VERSION}\"

# clang
CFLAGS = -target x86_64-windows-msvc -std=c99 -pedantic -Wall -Os ${CPPFLAGS}
LDFLAGS = -target x86_64-windows-msvc
CC = clang

# mingw64
# CFLAGS = -std=c99 -pedantic -Wall -Os ${CPPFLAGS}
# LDFLAGS = -s -mwindows -ldwmapi

ifeq ($(OS),Windows_NT)
	EXE=.exe
endif
