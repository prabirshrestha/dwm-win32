# dwm-win32 - dynamic window manager for win32
# See LICENSE file for copyright and license details.

include config.mk

SRC = dwm-win32.c
OBJ = ${SRC:.c=.o}

all: options dwm-win32

options:
	@echo dwm-win32 build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk

config.h:
	@echo creating $@ from config.def.h
	@cp config.def.h $@

dwm-win32: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f dwm-win32.exe ${OBJ} dwm-win32-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@mkdir -p dwm-win32-${VERSION}
	@cp -R LICENSE.txt Makefile README.txt config.def.h config.mk \
		${SRC} dwm-win32-${VERSION}
	@tar -cf dwm-win32-${VERSION}.tar dwm-win32-${VERSION}
	@gzip dwm-win32-${VERSION}.tar
	@rm -rf dwm-win32-${VERSION}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f dwm-win32 ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/dwm-win32
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@sed "s/VERSION/${VERSION}/g" < dwm-win32.1 > ${DESTDIR}${MANPREFIX}/man1/dwm-win32.1
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/dwm-win32.1

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/dwm-win32
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man1
	@rm -f ${DESTDIR}${MANPREFIX}/man1/dwm-win32.1

.PHONY: all options clean dist install uninstall
