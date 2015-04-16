PROG_CXX= opener
SRCS= opener.cpp

CFLAGS=-g -I/usr/local/include
LDFLAGS=-g -L/usr/local/lib

LDADD=-lPocoNet -lPocoUtil -lPocoFoundation

.include <bsd.prog.mk>
