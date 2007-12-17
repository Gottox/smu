# cmarkdown version
VERSION = 0.8.1

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# includes and libs
INCS = -I. -I/usr/include
LIBS = -L/usr/lib

# flags
CFLAGS = -Os -Wall -Werror ${INCS} -DVERSION=\"${VERSION}\"
LDFLAGS = ${LIBS}

# compiler
CC = cc
