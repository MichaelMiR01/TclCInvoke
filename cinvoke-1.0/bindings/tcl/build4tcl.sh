#./conf_tcl.sh ; \
LIBS="-L/usr/lib/x86_64-linux-gnu -ltclstub8.6 -L../../lib"
CPPFLAGS="-Wall -DMEMDEBUG -O2 -fPIC -shared -s -D_GNU_SOURCE -DHAVE_TCL_H -DUSE_TCL_STUBS -I../../lib -I/usr/include/tcl8.6/tcl-private -I/usr/include/tcl8.6/tcl-private/generic -I/usr/include/tcl8.6/tcl-private/unix"

echo gcc ${CPPFLAGS} -c cinvoke_tclcmd.c  ${LIBS} 
gcc ${CPPFLAGS} -c cinvoke_tclcmd.c  ${LIBS} 
gcc ${CPPFLAGS} -o cinvoke_tclcmd.so cinvoke_tclcmd.o ${LIBS} -L. -l:libcinvoke.a -Wl,-rpath=. 
gcc -shared -s -fPIC lib.c -o lib.so   
