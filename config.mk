# dwm-win32 version
VERSION = alpha2

CPPFLAGS = -DVERSION=\"${VERSION}\"

TARGET = x86_64-windows-msvc 

ifeq ($(32BIT),1)
	TARGET = i386-windows-msvc 
endif

# clang
CFLAGS = -target ${TARGET} -std=c99 -pedantic -Wall ${CPPFLAGS}
LDFLAGS = -target ${TARGET}
CC = clang

ifeq ($(DEBUG),1)
	CFLAGS += -g -gcodeview -O0
	LDFLAGS += -g
else
	CFLAGS += -Os -D NDEBUG
endif



ifeq ($(OS),Windows_NT)
	EXE=.exe
endif
