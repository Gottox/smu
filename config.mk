# cmarkdown version
VERSION = 0.8

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# includes and libs
INCS = -I. -I/usr/include
LIBS = -L/usr/lib

# flags
CFLAGS = -Os ${INCS} -DVERSION=\"${VERSION}\" -Wall
LDFLAGS = ${LIBS}

# compiler
CC = cc
