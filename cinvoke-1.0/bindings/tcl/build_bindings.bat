set CC=Z:\\host\\data\\tcl\\tcc_0.9.27-bin\\gcc\\bin\\gcc.exe -m32 -s 
set AR=Z:\\host\\data\\tcl\\tcc_0.9.27-bin\\gcc\\bin\\ar.exe -cr 

set CC=C:\d\Toolbox\tcl\tcc_0.9.27-bin\gcc\bin\gcc.exe -m32 -s
set AR=C:\d\Toolbox\tcl\tcc_0.9.27-bin\gcc\bin\ar.exe -cr 

rem set CC=C:\d\Toolbox\tcl\tcc_0.9.27-bin\tcc.exe -m32 
rem set AR=C:\d\Toolbox\tcl\tcc_0.9.27-bin\tcc.exe -ar 

set LIBS=-Llib -ltclstub86 -L../../lib
set CPPFLAGS=-O0 -m32 -shared -s -Wunused -DNOMEMDEBUG -D_GNU_SOURCE -DHAVE_TCL_H -DUSE_TCL_STUBS -I../../lib -Igeneric -Igeneric/win

del *.o
del *.a


%CC%  %CPPFLAGS% -shared -s -o cinvoke_tclcmd.dll cinvoke_tclcmd.c -L. -l:libcinvoke.a  %LIBS%

%CC% -DARCH_CL_X86_WIN -DCINVOKE_BUILD -DCINVOKE_DLL_EXPORT -shared -s lib.c -o"lib.dll"
