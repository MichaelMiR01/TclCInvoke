CC="$(pwd)/../tcc.exe -m32 -s " 
AR="$(pwd)/../tcc.exe -ar -cr " 

${CC} -c cinvoke.c -DARCH_TCC_X86_WIN -DCINVOKE_BUILD -DCINVOKE_DLL_EXPORT 
${CC} -c structure.c -DARCH_TCC_X86_WIN -DCINVOKE_BUILD -DCINVOKE_DLL_EXPORT
${CC} -c hashtable.c -DARCH_TCC_X86_WIN -DCINVOKE_BUILD -DCINVOKE_DLL_EXPORT
${CC} -c ./arch/tcc_x86_win.c -DARCH_TCC_X86_WIN -DCINVOKE_BUILD -DCINVOKE_DLL_EXPORT


${AR} libcinvoke.a cinvoke.o structure.o hashtable.o tcc_x86_win.o

${CC} -DARCH_TCC_X86_WIN -DCINVOKE_BUILD -DCINVOKE_DLL_EXPORT -shared -s  ./cinvoke.o ./tcc_x86_win.o ./structure.o ./hashtable.o -o"libcinvoke.dll"

cp libcinvoke.a libcinvoke_static.a
cd ..
cd test

${CC} -DARCH_TCC_X86_WIN -DCINVOKE_BUILD -DCINVOKE_DLL_EXPORT -shared -s lib.c -L../lib_unclobber -lcinvoke -o"lib.dll"
${CC} -DARCH_TCC_X86_WIN -DCINVOKE_BUILD -DCINVOKE_DLL_EXPORT  runtests.c -I../lib_unclobber -L. -L../lib_unclobber -llib -lcinvoke_static -o"runtests.exe"
 
