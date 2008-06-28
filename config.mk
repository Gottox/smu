# smu version
VERSION = 1.3

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# includes and libs
INCS = -I. -I/usr/include
LIBS = -L/usr/lib

# flags
CFLAGS = -g -O0 -Wall -Werror -ansi ${INCS} -DVERSION=\"${VERSION}\"
#CFLAGS = -fprofile-arcs -ftest-coverage -pg -g -O0 -Wall -Werror -ansi ${INCS} -DVERSION=\"${VERSION}\"
#CFLAGS = -Os -Wall -Werror -ansi ${INCS} -DVERSION=\"${VERSION}\"
#LDFLAGS = -fprofile-arcs -ftest-coverage -pg ${LIBS}
LDFLAGS = ${LIBS}

# compiler
CC = cc
