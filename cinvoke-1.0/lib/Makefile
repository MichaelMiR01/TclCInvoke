TARGET = libcinvoke.a
DYNLIB = libcinvoke.so
CC=gcc

WIN32=
XCC=i686-w64-mingw32-gcc 
XCPPFLAGS=-m32 $(CPPFLAGS)
ifdef WIN32
ARCH=gcc_x86_win
DYNLIB = libcinvoke.dll
else
ARCH=gcc_x64_unix
endif
all: $(TARGET) $(DYNLIB)

clean:
	rm -f *.o arch/*.o libcinvoke.a

SRCS = cinvoke.c structure.c hashtable.c arch/$(ARCH).c

OBJS = $(SRCS:.c=.o)
ifdef WIN32
$(TARGET): $(OBJS)
	rm -f $(TARGET) && ar rs $(TARGET) $(OBJS)
	ranlib $(TARGET)
	
$(DYNLIB): $(OBJS)
	rm -f $(DYNLIB) 
	$(XCC) -g -Wall -fPIC -DCINVOKE_BUILD -DARCH_GCC_X86_WIN  -static-libgcc -shared -s -o $(DYNLIB)  ./cinvoke.o ./arch/$(ARCH).o ./structure.o ./hashtable.o
	
.c.o:
	$(XCC) -g -Wall -fPIC -DCINVOKE_BUILD -DARCH_GCC_X86_WIN -c $< -o $@

cinvoke.o: cinvoke.c cinvoke.h cinvoke-arch.h arch/$(ARCH).h \
  cinvoke-private.h hashtable.h
structure.o: structure.c cinvoke.h cinvoke-arch.h arch/$(ARCH).h \
  cinvoke-private.h hashtable.h
hashtable.o: hashtable.c hashtable.h
arch/gcc_x64_unix.o: arch/$(ARCH).h arch/../cinvoke.h \
  arch/../cinvoke-arch.h \
  arch/../cinvoke-private.h arch/../hashtable.h
else
$(TARGET): $(OBJS)
	rm -f $(TARGET) && ar rs $(TARGET) $(OBJS)
	ranlib $(TARGET)
	
$(DYNLIB): $(OBJS)
	rm -f $(DYNLIB) 
	$(CC) -g -Wall -fPIC -DCINVOKE_BUILD -DARCH_GCC_X64_UNIX  -static-libgcc -shared -s -o $(DYNLIB)  ./cinvoke.o ./arch/$(ARCH).o ./structure.o ./hashtable.o  -ldl
	
.c.o:
	$(CC) -g -Wall -fPIC -DCINVOKE_BUILD -DARCH_GCC_X64_UNIX -c $< -o $@

cinvoke.o: cinvoke.c cinvoke.h cinvoke-arch.h arch/$(ARCH).h \
  cinvoke-private.h hashtable.h
structure.o: structure.c cinvoke.h cinvoke-arch.h arch/$(ARCH).h \
  cinvoke-private.h hashtable.h
hashtable.o: hashtable.c hashtable.h
arch/gcc_x64_unix.o: arch/$(ARCH).h arch/../cinvoke.h \
  arch/../cinvoke-arch.h \
  arch/../cinvoke-private.h arch/../hashtable.h
endif
