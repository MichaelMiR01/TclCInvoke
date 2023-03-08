set CC=Z:\\host\\data\\tcl\\tcc_0.9.27-bin\\gcc\\bin\\gcc.exe -m32 -s 
set AR=Z:\\host\\data\\tcl\\tcc_0.9.27-bin\\gcc\\bin\\ar.exe -cr 

%CC% -c cinvoke.c -DARCH_GCC_X86_WIN -DCINVOKE_BUILD -DCINVOKE_DLL_EXPORT
%CC% -c structure.c -DARCH_GCC_X86_WIN -DCINVOKE_BUILD -DCINVOKE_DLL_EXPORT
%CC% -c hashtable.c -DARCH_GCC_X86_WIN -DCINVOKE_BUILD -DCINVOKE_DLL_EXPORT
%CC% -c ./arch/gcc_x86_win.c -DARCH_GCC_X86_WIN -DCINVOKE_BUILD -DCINVOKE_DLL_EXPORT


%AR% libcinvoke.a cinvoke.o structure.o hashtable.o gcc_x86_win.o

%CC% -DARCH_GCC_X86_WIN -DCINVOKE_BUILD -DCINVOKE_DLL_EXPORT -static-libgcc -shared -s  ./cinvoke.o .\gcc_x86_win.o .\structure.o .\hashtable.o -o"libcinvoke.dll"

cd ..
cd test

%CC% -DARCH_GCC_X86_WIN -DCINVOKE_BUILD -DCINVOKE_DLL_EXPORT -shared -s lib.c -L../lib -lcinvoke -o"lib.dll"
%CC% -DARCH_GCC_X86_WIN -DCINVOKE_BUILD -DCINVOKE_DLL_EXPORT  runtests.c -I../lib -L. -L../lib -llib -lcinvoke -o"runtests.exe"
 
