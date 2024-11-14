# Set preferred compiler flags here
CC ?= cc
CFLAGS ?= -g -Wall -O2 -fPIC
CXX ?= c++
CXXFLAGS ?= -g -Wall -O2 -fPIC
CARGO ?= cargo
RUSTFLAGS ?= -g

# List of shared libraries to be built
LIBS = libcspinlock.so 

# This target builds all executables and shared libraries for tests
all: $(LIBS)

# Rule to build libcspinlock.so
libcspinlock.so: cspinlock.c
	$(CC) $(CFLAGS) -shared -o $@ $< -ldl

# Rule to build liblockhashmap.so
#liblockhashmap.so: lockhashmap.c
#	$(CC) $(CFLAGS) -shared -o $@ $< -ldl

# Rule to build liblockfreehashmap.so
#liblockfreehashmap.so: lockfreehashmap.c
#	$(CC) $(CFLAGS) -shared -o $@ $< -ldl

# Target to run tests
check: all
	$(MAKE) -C tests check

# Target to clean up generated files
clean:
	$(MAKE) -C tests clean
	rm -rf *.so* *.o
