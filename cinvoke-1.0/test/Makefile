
CFLAGS=-g -Wall -DCINVOKE_BUILD

WIN32=
ifdef WIN32
SHARED = lib.dll
CC=i686-w64-mingw32-gcc 
CPPFLAGS=-DARCH_GCC_X86_WIN -m32 -s $(CFLAGS) 
LFLAGS=
else
SHARED = lib.so
CC=gcc
CPPFLAGS=-DARCH_GCC_X64_UNIX $(CFLAGS) 
LFLAGS=-ldl
endif

all: $(SHARED) runtests

clean:
	rm -f lib.o $(SHARED) runtests

lib.o: lib.c
	$(CC)  $(CPPFLAGS)  -fpic -c lib.c -o lib.o

$(SHARED): lib.o
	$(CC) -shared -s -o $(SHARED) lib.o

runtests: runtests.c $(SHARED) ../lib/libcinvoke.a
	$(CC) -Wl,-rpath=. $(CPPFLAGS) -o runtests runtests.c -L../lib -I../lib -lcinvoke -lm $(LFLAGS)
