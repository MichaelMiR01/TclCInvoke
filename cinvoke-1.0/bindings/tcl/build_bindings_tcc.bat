rem set CC=Z:\\host\\data\\tcl\\tcc_0.9.27-bin\\tcc.exe -m32 -s 
rem set AR=Z:\\host\\data\\tcl\\tcc_0.9.27-bin\\tcc.exe -ar -cr 

rem set CC=C:\d\Toolbox\tcl\tcc_0.9.27-bin\gcc\bin\gcc.exe -m32 -s
rem set AR=C:\d\Toolbox\tcl\tcc_0.9.27-bin\gcc\bin\ar.exe -cr 

rem set CC=C:\d\Toolbox\tcl\tcc_0.9.27-bin\tcc.exe -m32 
rem set AR=C:\d\Toolbox\tcl\tcc_0.9.27-bin\tcc.exe -ar 

set CC=%cd%\..\..\tcc.exe -m32 -s 
set AR=%cd%\..\..\tcc.exe -ar -cr

set pwd=%cd%

set LIBS=-Llib -ltclstub86elf -L../../lib
set CPPFLAGS=-O2 -m32 -shared -s -Wunused -DNOMEMDEBUG -D_GNU_SOURCE -DHAVE_TCL_H -DUSE_TCL_STUBS -I../../lib -Igeneric -Igeneric/win

%CC%  %CPPFLAGS%  -o cinvoke_tclcmd.dll cinvoke_tclcmd.c %LIBS% -L. -lcinvoke_static
%CC%  -shared -s lib.c -o"lib.dll"
