set CC=Z:\\host\\data\\tcl\\tcc_0.9.27-bin\\gcc\\bin\\gcc.exe -m32 -s 
set AR=Z:\\host\\data\\tcl\\tcc_0.9.27-bin\\gcc\\bin\\ar.exe -cr 

rem set CC=C:\d\Toolbox\tcl\tcc_0.9.27-bin\gcc\bin\gcc.exe -m32 -s
rem set AR=C:\d\Toolbox\tcl\tcc_0.9.27-bin\gcc\bin\ar.exe -cr 

rem set CC=C:\d\Toolbox\tcl\tcc_0.9.27-bin\tcc.exe -m32 
rem set AR=C:\d\Toolbox\tcl\tcc_0.9.27-bin\tcc.exe -ar 

set LIBS=-Llib -ltclstub86
set CPPFLAGS=-O2 -m32 -shared -s -Wunused -DMEMDEBUG -D_GNU_SOURCE -DHAVE_TCL_H -DUSE_TCL_STUBS -I../../lib -Igeneric -Igeneric/win

del *.o
del *.a

%CC% %CPPFLAGS% -c cinvoke_tclcmd.c  %LIBS% 
%CC%  -shared -s -o cinvoke_tclcmd.dll cinvoke_tclcmd.o -L. -lcinvoke  %LIBS%
%CC%  -shared -s -o lib.dll lib.c
