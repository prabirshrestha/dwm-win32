# dwm-win32 version
VERSION = alpha2

CFLAGS = -target x86_64-windows-msvc -std=c99 -pedantic -Wall -Os -DVERSION=\"${VERSION}\"
LDFLAGS = -target x86_64-windows-msvc

ifeq ($(OS),Windows_NT)
	EXE=.exe
endif

# compiler and linker
CC = clang
