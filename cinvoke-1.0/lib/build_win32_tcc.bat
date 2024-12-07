set CC=%cd%\..\tcc.exe -m32 -s 
set AR=%cd%\..\tcc.exe -ar -cr

set pwd=%cd%

rem set CC=C:\d\Toolbox\tcl\tcc_0.9.27-bin\gcc\bin\gcc.exe -m32 -s
rem set AR=C:\d\Toolbox\tcl\tcc_0.9.27-bin\gcc\bin\ar.exe -cr 

rem set CC=C:\d\Toolbox\tcl\tcc_0.9.27-bin\tcc.exe -m32 
rem set AR=C:\d\Toolbox\tcl\tcc_0.9.27-bin\tcc.exe -ar 

del *.o
del *.a

%CC% -c cinvoke.c -DARCH_TCC_X86_WIN -DCINVOKE_BUILD -DCINVOKE_DLL_EXPORT
%CC% -c structure.c -DARCH_TCC_X86_WIN -DCINVOKE_BUILD -DCINVOKE_DLL_EXPORT
%CC% -c hashtable.c -DARCH_TCC_X86_WIN -DCINVOKE_BUILD -DCINVOKE_DLL_EXPORT
%CC% -c ./arch/tcc_x86_win.c -DARCH_TCC_X86_WIN -DCINVOKE_BUILD -DCINVOKE_DLL_EXPORT


%AR% libcinvoke.a cinvoke.o structure.o hashtable.o tcc_x86_win.o

%CC% -DARCH_TCC_X86_WIN -DCINVOKE_BUILD -DCINVOKE_DLL_EXPORT  -shared -s  ./cinvoke.o .\TCC_x86_win.o .\structure.o .\hashtable.o -o"libcinvoke.dll"

copy libcinvoke.a libcinvoke_static.a

pushd ..\test
del *.o

%CC% -DARCH_TCC_X86_WIN -DCINVOKE_BUILD -DCINVOKE_DLL_EXPORT -shared -s lib.c -L%pwd% -I%pwd% -o"lib.dll"
%CC% -DARCH_TCC_X86_WIN -DCINVOKE_BUILD -DCINVOKE_DLL_EXPORT  runtests.c -I. -L. -I%pwd% -L%pwd% -llib -lcinvoke_static -o"runtests.exe"

popd


:ende
echo ok
