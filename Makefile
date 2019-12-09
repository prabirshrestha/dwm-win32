# dwm-win32 - dynamic window manager for win32
# See LICENSE file for copyright and license details.

include config.mk

TARGET_EXEC ?= dwm-win32${EXE}

SRCS = dwm-win32.c
OBJS = ${SRCS:.c=.o}

all: options ${TARGET_EXEC}

options:
	@echo build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJS}: config.h config.mk bstack.c

config.h: config.def.h
	@echo creating $@ from config.def.h
	@cp config.def.h $@

${TARGET_EXEC}: ${OBJS}
	@echo CC -o $@ ${OBJS} ${LDFLAGS}
	@${CC} -o ${TARGET_EXEC} ${OBJS} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f ${OBJS} config.h ${TARGET_EXEC}

.PHONY: all options clean
