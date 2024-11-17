CC ?= cc
CFLAGS ?= -g -Wall -O2
CXX ?= c++
CXXFLAGS ?= -g -Wall -O2
CARGO ?= cargo
RUSTFLAGS ?= -g

all: libcspinlock.so liblockhashmap.so liblockfreehashmap.so

libcspinlock.so: cspinlock.c
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $<

liblockhashmap.so: lockhashmap.c libcspinlock.so
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $< -L. -lcspinlock

liblockfreehashmap.so: lockfreehashmap.c
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $<

check: all
	$(MAKE) -C tests check

clean:
	$(MAKE) -C tests clean
	rm -rf *.so* *.o