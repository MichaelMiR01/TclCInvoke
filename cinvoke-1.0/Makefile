all:
	cd lib && $(MAKE)
	cd test && $(MAKE)
	cp -f lib/arch/gcc_x64_unix.h lib/cinvoke-archspec.h

install:
	install -d /usr/local/include
	install -d /usr/local/lib
	install lib/cinvoke.h lib/cinvoke-arch.h lib/cinvoke-archspec.h \
		/usr/local/include
	install lib/libcinvoke.a /usr/local/lib

uninstall:
	rm -f /usr/local/include/cinvoke.h /usr/local/include/cinvoke-arch.h \
		/usr/local/include/cinvoke-archspec.h /usr/local/lib/libcinvoke.a

clean:
	cd lib && $(MAKE) clean
	cd test && $(MAKE) clean
