TARGET = libcinvoke.a
DYNLIB = libcinvoke_.so


all: $(TARGET) $(DYNLIB)

clean:
	rm -f *.o arch/*.o libcinvoke.a

SRCS = cinvoke.c structure.c hashtable.c arch/gcc_x64_unix.c

OBJS = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	rm -f $(TARGET) && ar rs $(TARGET) $(OBJS)
	ranlib $(TARGET)
	
$(DYNLIB): $(OBJS)
	rm -f $(DYNLIB) 
	gcc -g -Wall -fPIC -DCINVOKE_BUILD -DARCH_GCC_X64_UNIX  -static-libgcc -shared -s -o $(DYNLIB)  ./cinvoke.o ./arch/gcc_x64_unix.o ./structure.o ./hashtable.o  -ldl
	
.c.o:
	gcc -g -Wall -fPIC -DCINVOKE_BUILD -DARCH_GCC_X64_UNIX -c $< -o $@

cinvoke.o: cinvoke.c cinvoke.h cinvoke-arch.h arch/gcc_x64_unix.h \
  cinvoke-private.h hashtable.h
structure.o: structure.c cinvoke.h cinvoke-arch.h arch/gcc_x64_unix.h \
  cinvoke-private.h hashtable.h
hashtable.o: hashtable.c hashtable.h
arch/gcc_x64_unix.o: arch/gcc_x64_unix.h arch/../cinvoke.h \
  arch/../cinvoke-arch.h \
  arch/../cinvoke-private.h arch/../hashtable.h
