# libsmu - simple markup
# (c) 2007, 2008 Enno Boland

include config.mk

SRC    = smu.c
OBJ    = ${SRC:.c=.o}

all: options smu

options:
	@echo smu build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.mk

smu: ${OBJ}
	@echo LD $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f smu ${OBJ} ${LIBOBJ} smu-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@mkdir -p smu-${VERSION}
	@cp -R LICENSE Makefile config.mk smu.1 ${SRC} smu-${VERSION}
	@tar -cf smu-${VERSION}.tar smu-${VERSION}
	@gzip smu-${VERSION}.tar
	@rm -rf smu-${VERSION}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f smu ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/smu
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@sed "s/VERSION/${VERSION}/g" < smu.1 > ${DESTDIR}${MANPREFIX}/man1/smu.1
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/smu.1

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/smu
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man1
	@rm -f ${DESTDIR}${MANPREFIX}/man1/smu.1

.PHONY: all options clean dist install uninstall
