TARGET = cinvoke_tclcmd
DYNLIB = lib

WIN32=
CC=gcc

LIBS=-L../../lib -Llib
CPPFLAGS=-Wall -DMEMDEBUG -O2 -shared -s -D_GNU_SOURCE -DHAVE_TCL_H -DUSE_TCL_STUBS -I../../lib

XCC=i686-w64-mingw32-gcc 
XCPPFLAGS=-m32 $(CPPFLAGS) 
XINC=-Igeneric -Igeneric/win
#-D__USE_MINGW_ANSI_STDIO
all: $(TARGET)

clean:
	rm -f *.o 

SRCS = cinvoke_tclcmd.c lib.c

$(TARGET):
ifdef WIN32
	rm -f $(DYNLIB).o $(DYNLIB).dll $(TARGET).o $(TARGET).dll
	$(XCC)  $(XCPPFLAGS)  -o $(TARGET).dll $(TARGET).c $(XINC) $(LIBS) -ltclstub86  -L. -l:libcinvoke.a 
	$(XCC) -Wall -shared -s $(DYNLIB).c -o $(DYNLIB).dll 
else
	rm -f $(DYNLIB).o $(DYNLIB).so
	. ./conf_tcl.sh; \
	$(CC)  $(CPPFLAGS)  -fPIC -o $(TARGET).so $(TARGET).c $${TCC4TCL_CFLAGS} $${TCC4TCL_LIBS} ${LIBS}  -L. -l:libcinvoke.a -Wl,-rpath=. -ldl
	$(CC) -Wall -fPIC -shared -s $(DYNLIB).c -o $(DYNLIB).so
endif
