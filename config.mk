# dwm-win32 version
VERSION = alpha2

# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# flags
CPPFLAGS = -DVERSION=\"${VERSION}\"
CFLAGS = -std=c99 -pedantic -Wall -Os ${CPPFLAGS}
LDFLAGS = -s -mwindows

# compiler and linker
CC = gcc
