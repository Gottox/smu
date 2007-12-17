# cmarkdown
# (c) 2007 Enno Boland

include config.mk

SRC = cmarkdown.c
OBJ = ${SRC:.c=.o}

all: options cmarkdown

options:
	@echo cmarkdown build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.mk

cmarkdown: ${OBJ}
	@echo LD $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f cmarkdown ${OBJ} cmarkdown-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@mkdir -p cmarkdown-${VERSION}
	@cp -R LICENSE Makefile config.mk \
		cmarkdown.1 ${SRC} cmarkdown-${VERSION}
	@tar -cf cmarkdown-${VERSION}.tar cmarkdown-${VERSION}
	@gzip cmarkdown-${VERSION}.tar
	@rm -rf cmarkdown-${VERSION}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f cmarkdown ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/cmarkdown
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@sed "s/VERSION/${VERSION}/g" < cmarkdown.1 > ${DESTDIR}${MANPREFIX}/man1/cmarkdown.1
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/cmarkdown.1

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/cmarkdown
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man1
	@rm -f ${DESTDIR}${MANPREFIX}/man1/cmarkdown.1

.PHONY: all options clean dist install uninstall
