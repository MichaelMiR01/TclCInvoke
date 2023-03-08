TARGET = cinvoke_lua.so

all: $(TARGET)

test:
	lua test.lua

clean:
	rm -f *.o $(TARGET)

SRCS = cinvoke_lua.c

OBJS = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	gcc -shared -o $(TARGET) $(OBJS) -L../../lib -lcinvoke

.c.o:
	gcc -g -Wall -DCINVOKE_BUILD -DARCH_GCC_X64_UNIX -fpic -I../../lib -c $< -o $@

