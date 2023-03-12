TARGET = cinvoke_tclcmd
DYNLIB = lib

CC=gcc

LIBS=-ltclstub8.6 -L../../lib -Llib
CPPFLAGS=-Wall -DMEMDEBUG -O2 -fPIC -shared -s -D_GNU_SOURCE -DHAVE_TCL_H -DUSE_TCL_STUBS -I../../lib

#echo gcc ${CPPFLAGS} -c cinvoke_tclcmd.c  ${LIBS} 
#gcc ${CPPFLAGS} -c cinvoke_tclcmd.c  ${LIBS} 
#gcc ${CPPFLAGS} -o cinvoke_tclcmd.so cinvoke_tclcmd.o ${LIBS} -L. -l:libcinvoke.a -Wl,-rpath=. 

#$(CC) -shared -s -fPIC lib.c -o lib.so   

all: $(TARGET) $(DYNLIB)

clean:
	rm -f *.o *.so

SRCS = cinvoke_tclcmd.c lib.c


$(TARGET): $(SRCS)
	. ./conf_tcl.sh; \
	$(CC)  $(CPPFLAGS)  -o $(TARGET).so $(TARGET).c $${TCC4TCL_CFLAGS} $${TCC4TCL_LIBS} ${LIBS} -L. -l:libcinvoke.a -Wl,-rpath=. -ldl
	
$(DYNLIB): $(SRCS)
	rm -f $(DYNLIB).o $(DYNLIB).so
	$(CC) -g -Wall -fPIC -static-libgcc -shared -s $(DYNLIB).c -o $(DYNLIB).so -ldl
	